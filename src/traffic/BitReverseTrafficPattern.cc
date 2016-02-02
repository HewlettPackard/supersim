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
#include "traffic/BitReverseTrafficPattern.h"

#include <bits/bits.h>

#include <cassert>

BitReverseTrafficPattern::BitReverseTrafficPattern(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : BitPermutationTrafficPattern(_name, _parent, _numTerminals, _self,
                                   _settings) {
  dest_ = bits::reverse<u32>(self, bits::ceilLog2(numTerminals));
}

BitReverseTrafficPattern::~BitReverseTrafficPattern() {}

u32 BitReverseTrafficPattern::nextDestination() {
  return dest_;
}
