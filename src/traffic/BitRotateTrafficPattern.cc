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
#include "traffic/BitRotateTrafficPattern.h"

#include <bits/bits.h>

#include <cassert>

BitRotateTrafficPattern::BitRotateTrafficPattern(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : TrafficPattern(_name, _parent, _numTerminals, _self) {
  assert(bits::isPow2(numTerminals_));
  assert(_settings.isMember("direction"));
  assert(_settings["direction"].isString());
  std::string dir = _settings["direction"].asString();
  if (dir == "right") {
    dest_ = bits::rotateRight<u32>(self_, bits::floorLog2(numTerminals_));
  } else if (dir == "left") {
    dest_ = bits::rotateLeft<u32>(self_, bits::floorLog2(numTerminals_));
  } else {
    fprintf(stderr, "invalid direction spec: %s\n", dir.c_str());
    assert(false);
  }
}

BitRotateTrafficPattern::~BitRotateTrafficPattern() {}

u32 BitRotateTrafficPattern::nextDestination() {
  return dest_;
}
