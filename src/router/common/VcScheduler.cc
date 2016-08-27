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
#include "router/common/VcScheduler.h"

#include <cassert>
#include <cstring>

#include "allocator/AllocatorFactory.h"

VcScheduler::Client::Client() {}

VcScheduler::Client::~Client() {}

const s32 kAllocEvent = 123;

VcScheduler::VcScheduler(const std::string& _name, const Component* _parent,
                         u32 _numClients, u32 _totalVcs, Json::Value _settings)
    : Component(_name, _parent),
      numClients_(_numClients), totalVcs_(_totalVcs) {
  assert(numClients_ > 0 && numClients_ != U32_MAX);
  assert(totalVcs_ > 0 && totalVcs_ != U32_MAX);

  // create Client pointers and requested flags
  clients_.resize(numClients_, nullptr);
  clientRequested_.resize(numClients_, false);

  // create the VC used flags
  vcTaken_.resize(totalVcs_, false);

  // create arrays for allocator inputs and outputs
  requests_ = new bool[totalVcs_ * numClients_];
  memset(requests_, 0, sizeof(bool) * totalVcs_ * numClients_);
  metadatas_ = new u64[totalVcs_ * numClients_];
  grants_ = new bool[totalVcs_ * numClients_];

  // create the allocator
  allocator_ = AllocatorFactory::createAllocator(
      "Allocator", this, numClients_, totalVcs_, _settings["allocator"]);

  // map inputs and outputs to allocator
  for (u32 c = 0; c < numClients_; c++) {
    for (u32 v = 0; v < totalVcs_; v++) {
      allocator_->setRequest(c, v, &requests_[index(c, v)]);
      allocator_->setMetadata(c, v, &metadatas_[index(c, v)]);
      allocator_->setGrant(c, v, &grants_[index(c, v)]);
    }
  }

  // initialize state variables
  allocEventSet_ = false;
}

VcScheduler::~VcScheduler() {
  delete[] requests_;
  delete[] metadatas_;
  delete[] grants_;
  delete allocator_;
}

u32 VcScheduler::numClients() const {
  return numClients_;
}

u32 VcScheduler::totalVcs() const {
  return totalVcs_;
}

void VcScheduler::setClient(u32 _id, Client* _client) {
  assert(clients_.at(_id) == nullptr);
  clients_.at(_id) = _client;
}

void VcScheduler::request(u32 _client, u32 _vcIdx, u64 _metadata) {
  assert(gSim->epsilon() >= 1);
  assert(_client < numClients_);
  assert(_vcIdx < totalVcs_);

  // set the request
  u64 idx = index(_client, _vcIdx);
  requests_[idx] = true;
  metadatas_[idx] = _metadata;
  clientRequested_[_client] = true;

  // ensure there is an event set to perform scheduling
  if (!allocEventSet_) {
    allocEventSet_ = true;
    addEvent(gSim->futureCycle(1), 0, nullptr, kAllocEvent);
  }
}

void VcScheduler::releaseVc(u32 _vcIdx) {
  assert(gSim->epsilon() >= 1);
  assert(vcTaken_.at(_vcIdx) == true);
  vcTaken_.at(_vcIdx) = false;
}

void VcScheduler::processEvent(void* _event, s32 _type) {
  assert(_type == kAllocEvent);
  assert(gSim->epsilon() == 0);
  allocEventSet_ = false;

  // check VC availability, mask out unavailable VC requests
  for (u32 c = 0; c < numClients_; c++) {
    if (clientRequested_[c]) {
      for (u32 v = 0; v < totalVcs_; v++) {
        u64 idx = index(c, v);
        if (requests_[idx] && vcTaken_[v]) {
          requests_[idx] = false;
        }
      }
    }
  }

  // clear the grants (must do before allocate() call)
  memset(grants_, false, sizeof(bool) * totalVcs_ * numClients_);

  // run the allocator
  allocator_->allocate();

  // deliver responses, mark used VCs, reset requests
  for (u32 c = 0; c < numClients_; c++) {
    if (clientRequested_[c]) {
      clientRequested_[c] = false;
      u32 granted = U32_MAX;
      for (u32 v = 0; v < totalVcs_; v++) {
        u64 idx = index(c, v);

        // multiple grants to the same client? BAD
        assert(!((granted != U32_MAX) && (grants_[idx])));

        // check for granted
        if ((granted == U32_MAX) && (grants_[idx])) {
          granted = v;
          assert(vcTaken_[v] == false);
          vcTaken_[v] = true;
        }
        requests_[idx] = false;
      }
      clients_[c]->vcSchedulerResponse(granted);
    }
  }
}

u64 VcScheduler::index(u64 _client, u64 _vcIdx) const {
  // this indexing contiguously places resources
  return (totalVcs_ * _client) + _vcIdx;
}
