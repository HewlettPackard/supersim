/*
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "architecture/CrossbarScheduler.h"

#include <cassert>
#include <cstring>

#include "allocator/Allocator.h"
#include "types/Packet.h"

static bool warningIssued = false;

CrossbarScheduler::Client::Client() {}

CrossbarScheduler::Client::~Client() {}

CrossbarScheduler::CrossbarScheduler(
    const std::string& _name, const Component* _parent, u32 _numClients,
    u32 _totalVcs, u32 _crossbarPorts, u32 _globalVcOffset,
    Simulator::Clock _clock, Json::Value _settings)
    : Component(_name, _parent), numClients_(_numClients),
      totalVcs_(_totalVcs), crossbarPorts_(_crossbarPorts),
      globalVcOffset_(_globalVcOffset), clock_(_clock),
      fullPacket_(_settings["full_packet"].asBool()),
      packetLock_(_settings["packet_lock"].asBool()),
      idleUnlock_(_settings["idle_unlock"].asBool()) {
  assert(!_settings["full_packet"].isNull());
  assert(!_settings["packet_lock"].isNull());
  assert(!_settings["idle_unlock"].isNull());
  if (!warningIssued && !fullPacket_ && packetLock_ && !idleUnlock_) {
    printf("**************************************************************\n"
           "** WARNING!!!!!!! Packet-Channel Flit-Buffer Flow Control   **\n"
           "** causes deadlock if VCs are being used to avoid deadlock. **\n"
           "**************************************************************\n");
    warningIssued = true;
  }
  if (idleUnlock_) {
    assert(packetLock_);
  }

  assert(numClients_ > 0 && numClients_ != U32_MAX);
  assert(totalVcs_ > 0 && totalVcs_ != U32_MAX);
  assert(crossbarPorts_ > 0 && crossbarPorts_ != U32_MAX);

  // create Client pointers and requested flags
  clients_.resize(numClients_, nullptr);
  clientRequestPorts_.resize(numClients_, U32_MAX);
  clientRequestVcs_.resize(numClients_, U32_MAX);
  clientRequestFlits_.resize(numClients_, nullptr);

  // create the credit counters
  credits_.resize(totalVcs_, 0);
  maxCredits_.resize(totalVcs_, 0);

  // create arrays for allocator inputs and outputs
  requests_ = new bool[crossbarPorts_ * numClients_];
  memset(requests_, false, crossbarPorts_ * numClients_);
  metadatas_ = new u64[crossbarPorts_ * numClients_];
  grants_ = new bool[crossbarPorts_ * numClients_];

  // create arrays for handling port locks
  anyRequests_.resize(crossbarPorts_, false);
  portLocks_.resize(crossbarPorts_, U32_MAX);

  // create the allocator
  allocator_ = Allocator::create(
      "Allocator", this, numClients_, crossbarPorts_, _settings["allocator"]);

  // map inputs and outputs to allocator
  for (u32 c = 0; c < numClients_; c++) {
    for (u32 p = 0; p < crossbarPorts_; p++) {
      allocator_->setRequest(c, p, &requests_[index(c, p)]);
      allocator_->setMetadata(c, p, &metadatas_[index(c, p)]);
      allocator_->setGrant(c, p, &grants_[index(c, p)]);
    }
  }

  // initialize state variables
  eventAction_ = EventAction::NONE;
}

CrossbarScheduler::~CrossbarScheduler() {
  delete[] requests_;
  delete[] metadatas_;
  delete[] grants_;
  delete allocator_;
}

u32 CrossbarScheduler::numClients() const {
  return numClients_;
}

u32 CrossbarScheduler::totalVcs() const {
  return totalVcs_;
}

u32 CrossbarScheduler::crossbarPorts() const {
  return crossbarPorts_;
}

void CrossbarScheduler::setClient(u32 _id, Client* _client) {
  assert(clients_.at(_id) == nullptr);
  clients_.at(_id) = _client;
}

void CrossbarScheduler::addCreditWatcher(CreditWatcher* _watcher) {
  watchers_.push_back(_watcher);
}

void CrossbarScheduler::request(u32 _client, u32 _port, u32 _vcIdx,
                                Flit* _flit) {
  assert(gSim->epsilon() >= 1);
  assert(_client < numClients_);
  assert(clientRequestPorts_[_client] == U32_MAX);
  assert(clientRequestVcs_[_client] == U32_MAX);
  assert(clientRequestFlits_[_client] == nullptr);
  assert(_vcIdx < totalVcs_);
  assert(_port < crossbarPorts_);

  // set request
  clientRequestPorts_[_client] = _port;
  clientRequestVcs_[_client] = _vcIdx;
  clientRequestFlits_[_client] = _flit;
  anyRequests_[_port] = true;
  u64 idx = index(_client, _port);
  requests_[idx] = true;
  metadatas_[idx] = _flit->packet()->getMetadata();

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::RUNALLOC;
    addEvent(gSim->futureCycle(clock_, 1), 0, nullptr, 0);
  } else if (eventAction_ == EventAction::CREDITS) {
    eventAction_ = EventAction::RUNALLOC;
  }
}

void CrossbarScheduler::initCreditCount(u32 _vcIdx, u32 _credits) {
  assert(_vcIdx < totalVcs_);
  credits_[_vcIdx] = _credits;
  maxCredits_[_vcIdx] = _credits;

  // update watchers
  for (CreditWatcher* watcher : watchers_) {
    watcher->initCredits(globalVcOffset_ + _vcIdx, _credits);
  }
}

void CrossbarScheduler::incrementCreditCount(u32 _vcIdx) {
  assert(gSim->epsilon() >= 1);
  assert(_vcIdx < totalVcs_);

  // add increment value to VC
  incrCredits_[_vcIdx]++;

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::CREDITS;
    addEvent(gSim->futureCycle(clock_, 1), 0, nullptr, 0);
  }

  // update watchers
  for (CreditWatcher* watcher : watchers_) {
    watcher->incrementCredit(globalVcOffset_ + _vcIdx);
  }
}

void CrossbarScheduler::decrementCreditCount(u32 _vcIdx) {
  assert(_vcIdx < totalVcs_);

  // decrement the credit count
  assert(credits_[_vcIdx] > 0);
  credits_[_vcIdx]--;

  // update watchers
  for (CreditWatcher* watcher : watchers_) {
    watcher->decrementCredit(globalVcOffset_ + _vcIdx);
  }
}

u32 CrossbarScheduler::getCreditCount(u32 _vcIdx) const {
  assert(_vcIdx < totalVcs_);
  return credits_[_vcIdx];
}

void CrossbarScheduler::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() == 0);
  assert(eventAction_ != EventAction::NONE);

  // apply all credit incrementations needed
  for (auto it = incrCredits_.cbegin(); it != incrCredits_.cend(); ++it) {
    u32 vc = it->first;
    u32 incr = it->second;
    assert(vc < totalVcs_);
    credits_[vc] += incr;
    assert(credits_[vc] <= maxCredits_[vc]);
  }
  incrCredits_.clear();

  // if required, run the allocator
  if (eventAction_ == EventAction::RUNALLOC) {
    // check credit counts for each request
    //  when credits aren't sufficient, disable the request
    for (u32 c = 0; c < numClients_; c++) {
      if (clientRequestPorts_[c] != U32_MAX) {
        u32 port = clientRequestPorts_[c];
        u32 vc = clientRequestVcs_[c];
        u64 idx = index(c, port);

        if (fullPacket_) {
          // packet-buffer flow control
          const Flit* flit = clientRequestFlits_[c];
          if (flit->isHead()) {
            u32 packetSize = flit->packet()->numFlits();
            assert(maxCredits_[vc] >= packetSize);  // buffer is large enough
            if (requests_[idx] && credits_[vc] < packetSize) {
              requests_[idx] = false;
            }
          }
        } else {
          // flit-buffer flow control
          if (requests_[idx] && credits_[vc] == 0) {
            requests_[idx] = false;
          }
        }
      }
    }

    if (packetLock_) {
      // perform the lock request filtering algorithm
      for (u32 p = 0; p < crossbarPorts_; p++) {
        // the lock has to be active and there must be at least one request
        if (portLocks_[p] != U32_MAX && anyRequests_[p]) {
          // retrieve the lock owner's info
          u32 owner = portLocks_[p];
          u32 ownerIndex = index(owner, p);

          // determine if idle unlock is applicable
          if (idleUnlock_ && !requests_[ownerIndex]) {
            // the owner isn't requesting and idle unlock is enabled
            //  disable the port lock
            portLocks_[p] = U32_MAX;
          }

          // determine if the requests need to be deactivated
          if (portLocks_[p] != U32_MAX) {
            // deactivate other requests
            for (u32 c = 0; c < numClients_; c++) {
              if (c != owner) {
                u32 otherIndex = index(c, p);
                requests_[otherIndex] = false;
              }
            }
          }
        }
      }
    }

    // clear the any request vector
    for (u32 p = 0; p < crossbarPorts_; p++) {
      anyRequests_[p] = false;
    }

    // clear the grants (must do before allocate() call)
    memset(grants_, false, sizeof(bool) * numClients_ * crossbarPorts_);

    // run the allocator
    allocator_->allocate();

    // deliver responses, reset requests, if required lock ports
    for (u32 c = 0; c < numClients_; c++) {
      if (clientRequestPorts_[c] != U32_MAX) {
        u32 port = clientRequestPorts_[c];
        clientRequestPorts_[c] = U32_MAX;
        u32 vc = clientRequestVcs_[c];
        clientRequestVcs_[c] = U32_MAX;
        const Flit* flit = clientRequestFlits_[c];
        clientRequestFlits_[c] = nullptr;
        u64 idx = index(c, port);

        u32 granted = U32_MAX;
        if (grants_[idx]) {
          granted = port;
          assert(credits_[vc] > 0);

          // if needed, lock the port
          if (packetLock_) {
            // handle port locking
            portLocks_[port] = flit->isTail() ? U32_MAX : c;
          }
        }
        requests_[idx] = false;

        clients_[c]->crossbarSchedulerResponse(granted, vc);
      }
    }
  }

  // reset event
  eventAction_ = EventAction::NONE;
}

u64 CrossbarScheduler::index(u64 _client, u64 _port) const {
  // this indexing contiguously places resources
  return (crossbarPorts_ * _client) + _port;
}
