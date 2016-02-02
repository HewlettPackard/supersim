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

CrossbarScheduler::Client::Client() {}

CrossbarScheduler::Client::~Client() {}

CrossbarScheduler::CrossbarScheduler(
    const std::string& _name, const Component* _parent, u32 _numClients,
    u32 _numVcs, u32 _numPorts, Json::Value _settings)
    : Component(_name, _parent), numClients_(_numClients), numVcs_(_numVcs),
      numPorts_(_numPorts) {
  assert(numClients_ > 0 && numClients_ != U32_MAX);
  assert(numVcs_ > 0 && numVcs_ != U32_MAX);
  assert(numPorts_ > 0 && numPorts_ != U32_MAX);

  // create Client pointers and requested flags
  clients_.resize(numClients_, nullptr);
  clientRequestPorts_.resize(numClients_, U32_MAX);
  clientRequestVcs_.resize(numClients_, U32_MAX);

  // create the credit counters
  credits_.resize(numVcs_, 0);
  maxCredits_.resize(numVcs_, 0);

  // create arrays for allocator inputs and outputs
  requests_ = new bool[numPorts_ * numClients_];
  memset(requests_, 0, sizeof(bool) * numPorts_ * numClients_);
  metadatas_ = new u64[numPorts_ * numClients_];
  grants_ = new bool[numPorts_ * numClients_];

  // create the allocator
  allocator_ = AllocatorFactory::createAllocator(
      "Allocator", this, numClients_, numPorts_, _settings["allocator"]);

  // map inputs and outputs to allocator
  for (u32 c = 0; c < numClients_; c++) {
    for (u32 p = 0; p < numPorts_; p++) {
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

u32 CrossbarScheduler::numVcs() const {
  return numVcs_;
}

u32 CrossbarScheduler::numPorts() const {
  return numPorts_;
}

void CrossbarScheduler::setClient(u32 _id, Client* _client) {
  assert(clients_.at(_id) == nullptr);
  clients_.at(_id) = _client;
}

void CrossbarScheduler::request(u32 _client, u32 _port, u32 _vc,
                                u32 _metadata) {
  assert(gSim->epsilon() >= 1);
  assert(_client < numClients_);
  assert(clientRequestPorts_[_client] == U32_MAX);
  assert(clientRequestVcs_[_client] == U32_MAX);
  assert(_vc < numVcs_);
  assert(_port < numPorts_);

  // set request
  clientRequestPorts_[_client] = _port;
  clientRequestVcs_[_client] = _vc;
  u64 idx = index(_client, _port);
  requests_[idx] = true;
  metadatas_[idx] = _metadata;

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::RUNALLOC;
    addEvent(gSim->futureCycle(1), 0, nullptr, 0);
  } else if (eventAction_ == EventAction::CREDITS) {
    eventAction_ = EventAction::RUNALLOC;
  }
}

void CrossbarScheduler::initCreditCount(u32 _vc, u32 _credits) {
  assert(_vc < numVcs_);
  credits_[_vc] = _credits;
  maxCredits_[_vc] = _credits;
}

void CrossbarScheduler::incrementCreditCount(u32 _vc) {
  assert(gSim->epsilon() >= 1);
  assert(_vc < numVcs_);

  // add increment value to VC
  incrCredits_[_vc]++;

  // upgrade event
  if (eventAction_ == EventAction::NONE) {
    eventAction_ = EventAction::CREDITS;
    addEvent(gSim->futureCycle(1), 0, nullptr, 0);
  }
}

void CrossbarScheduler::decrementCreditCount(u32 _vc) {
  assert(_vc < numVcs_);

  // decrement the credit count
  assert(credits_[_vc] > 0);
  credits_[_vc]--;
}

u32 CrossbarScheduler::getCreditCount(u32 _vc) const {
  assert(_vc < numVcs_);
  return credits_[_vc];
}

void CrossbarScheduler::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() == 0);
  assert(eventAction_ != EventAction::NONE);

  // apply all credit incrementations needed
  for (auto it = incrCredits_.cbegin(); it != incrCredits_.cend(); ++it) {
    u32 vc = it->first;
    u32 incr = it->second;
    assert(vc < numVcs_);
    credits_[vc] += incr;
    assert(credits_[vc] <= maxCredits_[vc]);
  }
  incrCredits_.clear();

  // if required, run the allocator
  if (eventAction_ == EventAction::RUNALLOC) {
    // check credit counts for each request
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

    // clear the grants (must do before allocate() call)
    memset(grants_, false, sizeof(bool) * numClients_ * numPorts_);

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
  return (numPorts_ * _client) + _port;
}
