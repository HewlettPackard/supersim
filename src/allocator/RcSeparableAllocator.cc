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
#include "allocator/RcSeparableAllocator.h"

#include <factory/Factory.h>

#include <cassert>

#include "arbiter/Arbiter.h"

RcSeparableAllocator::RcSeparableAllocator(
    const std::string& _name, const Component* _parent,
    u32 _numClients, u32 _numResources, Json::Value _settings)
    : Allocator(_name, _parent, _numClients, _numResources, _settings) {
  // pointer arrays
  requests_ = new bool*[numClients_ * numResources_];
  metadatas_ = new u64*[numClients_ * numResources_];
  intermediates_ = new bool[numClients_ * numResources_];
  grants_ = new bool*[numClients_ * numResources_];

  // use vector to hold arbiter pointers
  resourceArbiters_.resize(numResources_, nullptr);
  clientArbiters_.resize(numClients_, nullptr);

  // instantiate the resource arbiters
  for (u32 r = 0; r < numResources_; r++) {
    std::string name = "ArbiterR" + std::to_string(r);
    resourceArbiters_[r] = Arbiter::create(
        name, this, numClients_, _settings["resource_arbiter"]);
  }

  // instantiate the client arbiters
  for (u32 c = 0; c < numClients_; c++) {
    std::string name = "ArbiterC" + std::to_string(c);
    clientArbiters_[c] = Arbiter::create(
        name, this, numResources_, _settings["client_arbiter"]);
  }

  // map intermediate request signals to arbiters
  for (u32 r = 0; r < numResources_; r++) {
    for (u32 c = 0; c < numClients_; c++) {
      bool* i = &intermediates_[index(c, r)];
      resourceArbiters_[r]->setGrant(c, i);
      clientArbiters_[c]->setRequest(r, i);
    }
  }

  // parse settings
  iterations_ = _settings["iterations"].asUInt();
  assert(iterations_ > 0);
  slipLatch_ = _settings["slip_latch"].asBool();
}

RcSeparableAllocator::~RcSeparableAllocator() {
  for (u32 r = 0; r < numResources_; r++) {
    delete resourceArbiters_[r];
  }
  for (u32 c = 0; c < numClients_; c++) {
    delete clientArbiters_[c];
  }
  delete[] requests_;
  delete[] metadatas_;
  delete[] intermediates_;
  delete[] grants_;
}

void RcSeparableAllocator::setRequest(u32 _client, u32 _resource,
                                      bool* _request) {
  requests_[index(_client, _resource)] = _request;
  resourceArbiters_[_resource]->setRequest(_client, _request);
}

void RcSeparableAllocator::setMetadata(u32 _client, u32 _resource,
                                       u64* _metadata) {
  metadatas_[index(_client, _resource)] = _metadata;
  resourceArbiters_[_resource]->setMetadata(_client, _metadata);
  clientArbiters_[_client]->setMetadata(_resource, _metadata);
}

void RcSeparableAllocator::setGrant(u32 _client, u32 _resource, bool* _grant) {
  grants_[index(_client, _resource)] = _grant;
  clientArbiters_[_client]->setGrant(_resource, _grant);
}

void RcSeparableAllocator::allocate() {
  for (u32 remaining = iterations_; remaining > 0; remaining--) {
    // clear the intermediate stage
    for (u32 r = 0; r < numResources_; r++) {
      for (u32 c = 0; c < numClients_; c++) {
        intermediates_[index(c, r)] = false;
      }
    }

    // run the resource arbiters
    for (u32 r = 0; r < numResources_; r++) {
      resourceArbiters_[r]->arbitrate();

      // perform arbiter state latching
      if (!slipLatch_) {
        // regular latch always algorithm
        resourceArbiters_[r]->latch();
      }
    }

    // run the client arbiters
    for (u32 c = 0; c < numClients_; c++) {
      u32 winningResource = clientArbiters_[c]->arbitrate();
      if (winningResource != U32_MAX) {
        // remove the requests from this client
        for (u32 r = 0; r < numResources_; r++) {
          *requests_[index(c, r)] = false;
        }
        // remove the requests for this resource
        for (u32 c = 0; c < numClients_; c++) {
          *requests_[index(c, winningResource)] = false;
        }
      }

      // perform arbiter state latching
      if (slipLatch_) {
        // slip latching (iSLIP algorithm)
        if (winningResource != U32_MAX) {
          clientArbiters_[c]->latch();
          resourceArbiters_[winningResource]->latch();
        }
      } else {
        // regular latch always algorithm
        clientArbiters_[c]->latch();
      }
    }
  }
}

u64 RcSeparableAllocator::index(u64 _client, u64 _resource) const {
  return (numClients_ * _resource) + _client;
}

registerWithFactory("rc_separable", Allocator, RcSeparableAllocator,
                    ALLOCATOR_ARGS);
