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
#include "allocator/AllocatorFactory.h"

#include <cassert>

#include "allocator/CrSeparableAllocator.h"
#include "allocator/RcSeparableAllocator.h"
#include "allocator/RSeparableAllocator.h"

Allocator* AllocatorFactory::createAllocator(
    const std::string& _name, const Component* _parent,
    u32 _numClients, u32 _numResources, Json::Value _settings) {
  std::string type = _settings["type"].asString();

  if (type == "cr_separable") {
    return new CrSeparableAllocator(_name, _parent, _numClients, _numResources,
                                    _settings);
  } else if (type == "rc_separable") {
    return new RcSeparableAllocator(_name, _parent, _numClients, _numResources,
                                    _settings);
  } else if (type == "r_separable") {
    return new RSeparableAllocator(_name, _parent, _numClients, _numResources,
                                   _settings);
  } else {
    fprintf(stderr, "unknown Allocator 'type': %s\n", type.c_str());
    assert(false);
  }
}
