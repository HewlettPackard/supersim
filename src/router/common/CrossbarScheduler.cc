/*
 * Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "router/common/CrossbarScheduler.h"

#include <cassert>
#include <cstring>

#include "allocator/AllocatorFactory.h"
#include "types/Packet.h"

CrossbarScheduler::Client::Client() {}

CrossbarScheduler::Client::~Client() {}

CrossbarScheduler::CrossbarScheduler(
    const std::string& _name, const Component* _parent, u32 _numClients,
    u32 _totalVcs, u32 _crossbarPorts, Json::Value _settings)
    : Component(_name, _parent), numClients_(_numClients),
      totalVcs_(_totalVcs), crossbarPorts_(_crossbarPorts),
      packetLock_(_settings["packet_lock"].asBool()),
      idleUnlock_(_settings["idle_unlock"].asBool()) {
  assert(!_settings["packet_lock"].isNull());
  assert(!_settings["idle_unlock"].isNull());
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
  clientRequestTails_.resize(numClients_, false);

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
  portLocks_.resize(crossbarPorts_, false);

  // create the allocator
  allocator_ = AllocatorFactory::createAllocator(
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

void CrossbarScheduler::request(u32 _client, u32 _port, u32 _vcIdx,
                                Flit* _flit) {
  assert(gSim->epsilon() >= 1);
  assert(_client < numClients_);
  assert(clientRequestPorts_[_client] == U32_MAX);
  assert(clientRequestVcs_[_client] == U32_MAX);
  assert(_vcIdx < totalVcs_);
  assert(_port < crossbarPorts_);

  // set request
  clientRequestPorts_[_client] = _port;
  clientRequestVcs_[_client] = _vcIdx;
  clientRequestTails_[_client] = _flit->isTail();
  anyRequests_[_port] = true;
  u64 idx = index(_client, _port);
  requests_[idx] = true;
  metadatas_[idx] = _flit->getPacket()->getMetadata();

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::RUNALLOC;
    addEvent(gSim->futureCycle(1), 0, nullptr, 0);
  } else if (eventAction_ == EventAction::CREDITS) {
    eventAction_ = EventAction::RUNALLOC;
  }
}

void CrossbarScheduler::initCreditCount(u32 _vcIdx, u32 _credits) {
  assert(_vcIdx < totalVcs_);
  credits_[_vcIdx] = _credits;
  maxCredits_[_vcIdx] = _credits;
}

void CrossbarScheduler::incrementCreditCount(u32 _vcIdx) {
  assert(gSim->epsilon() >= 1);
  assert(_vcIdx < totalVcs_);

  // add increment value to VC
  incrCredits_[_vcIdx]++;

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::CREDITS;
    addEvent(gSim->futureCycle(1), 0, nullptr, 0);
  }
}

void CrossbarScheduler::decrementCreditCount(u32 _vcIdx) {
  assert(_vcIdx < totalVcs_);

  // decrement the credit count
  assert(credits_[_vcIdx] > 0);
  credits_[_vcIdx]--;
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
    //  when credits don't exist, disable the request
    for (u32 c = 0; c < numClients_; c++) {
      if (clientRequestPorts_[c] != U32_MAX) {
        u32 port = clientRequestPorts_[c];
        u32 vc = clientRequestVcs_[c];
        u64 idx = index(c, port);
        if (requests_[idx] && credits_[vc] == 0) {
          requests_[idx] = false;
        }
      }
    }

    // handle the locking algorithm
    if (packetLock_) {
      // perform the port locking mechanism
      for (u32 p = 0; p < crossbarPorts_; p++) {
        // the lock has to be active and there must be at least one request
        if (portLocks_[p] != U32_MAX && anyRequests_[p]) {
          // determine if the owner is requesting
          u32 owner = portLocks_[p];
          u32 ownerIndex = index(owner, p);
          if (requests_[ownerIndex]) {
            // deactivate other requests
            for (u32 c = 0; c < numClients_; c++) {
              if (c != owner) {
                u32 otherIndex = index(c, p);
                requests_[otherIndex] = false;
              }
            }

            // deactivate the port lock if the request is for a tail flit
            if (clientRequestTails_[owner]) {
              portLocks_[p] = U32_MAX;
            }
          } else if (idleUnlock_) {
            // the owner isn't requesting and idle unlock is enabled, disable
            //  the port lock
            portLocks_[p] = U32_MAX;
          }
        }
      }

      // clear the any request vector
      for (u32 p = 0; p < crossbarPorts_; p++) {
        anyRequests_[p] = false;
      }
    }

    // clear the grants (must do before allocate() call)
    memset(grants_, false, sizeof(bool) * numClients_ * crossbarPorts_);

    // run the allocator
    allocator_->allocate();

    // deliver responses, reset requests
    for (u32 c = 0; c < numClients_; c++) {
      if (clientRequestPorts_[c] != U32_MAX) {
        u32 port = clientRequestPorts_[c];
        clientRequestPorts_[c] = U32_MAX;
        u32 vc = clientRequestVcs_[c];
        clientRequestVcs_[c] = U32_MAX;
        u64 idx = index(c, port);

        u32 granted = U32_MAX;
        if (grants_[idx]) {
          granted = port;
          assert(credits_[vc] > 0);
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
