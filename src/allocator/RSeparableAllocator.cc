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
#include "allocator/RSeparableAllocator.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "arbiter/Arbiter.h"

RSeparableAllocator::RSeparableAllocator(
    const std::string& _name, const Component* _parent,
    u32 _numClients, u32 _numResources, Json::Value _settings)
    : Allocator(_name, _parent, _numClients, _numResources, _settings) {
  // pointer arrays
  requests_.resize(numClients_ * numResources_, nullptr);
  metadatas_.resize(numClients_ * numResources_, nullptr);
  grants_.resize(numClients_ * numResources_, nullptr);

  // use vector to hold arbiter pointers
  resourceArbiters_.resize(numResources_, nullptr);

  // instantiate the resource arbiters
  for (u32 r = 0; r < numResources_; r++) {
    std::string name = "ArbiterR" + std::to_string(r);
    resourceArbiters_[r] = Arbiter::create(
        name, this, numClients_, _settings["resource_arbiter"]);
  }

  // parse settings
  slipLatch_ = _settings["slip_latch"].asBool();
}

RSeparableAllocator::~RSeparableAllocator() {
  for (u32 r = 0; r < numResources_; r++) {
    delete resourceArbiters_[r];
  }
}

void RSeparableAllocator::setRequest(u32 _client, u32 _resource,
                                     bool* _request) {
  requests_.at(index(_client, _resource)) = _request;
  resourceArbiters_.at(_resource)->setRequest(_client, _request);
}

void RSeparableAllocator::setMetadata(u32 _client, u32 _resource,
                                      u64* _metadata) {
  metadatas_.at(index(_client, _resource)) = _metadata;
  resourceArbiters_.at(_resource)->setMetadata(_client, _metadata);
}

void RSeparableAllocator::setGrant(u32 _client, u32 _resource, bool* _grant) {
  grants_.at(index(_client, _resource)) = _grant;
  resourceArbiters_.at(_resource)->setGrant(_client, _grant);
}

void RSeparableAllocator::allocate() {
  // run the resource arbiters
  for (u32 r = 0; r < numResources_; r++) {
    u32 winningClient = resourceArbiters_[r]->arbitrate();

    // perform arbiter state latching
    if (!slipLatch_ || winningClient != U32_MAX) {
      resourceArbiters_[r]->latch();
    }
  }
}

u64 RSeparableAllocator::index(u64 _client, u64 _resource) const {
  return (numClients_ * _resource) + _client;
}

registerWithObjectFactory("r_separable", Allocator, RSeparableAllocator,
                          ALLOCATOR_ARGS);
