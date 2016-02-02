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
#include "arbiter/Arbiter.h"

#include <cassert>

Arbiter::Arbiter(
    const std::string& _name, const Component* _parent, u32 _size)
    : Component(_name, _parent), size_(_size) {
  assert(size_ > 0);
  requests_.resize(size_, nullptr);
  metadatas_.resize(size_, nullptr);
  grants_.resize(size_, nullptr);
}

Arbiter::~Arbiter() {}

u32 Arbiter::size() const {
  return size_;
}

void Arbiter::setRequest(u32 _port, const bool* _request) {
  requests_.at(_port) = _request;
}

void Arbiter::setMetadata(u32 _port, const u64* _metadata) {
  metadatas_.at(_port) = _metadata;
}

void Arbiter::setGrant(u32 _port, bool* _grant) {
  grants_.at(_port) = _grant;
}

void Arbiter::latch() {}
