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
#include "traffic/continuous/BitReverseCTP.h"

#include <bits/bits.h>
#include <factory/ObjectFactory.h>

#include <cassert>

BitReverseCTP::BitReverseCTP(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  assert(bits::isPow2(numTerminals_));
  dest_ = bits::reverse<u32>(self_, bits::ceilLog2(numTerminals_));
}

BitReverseCTP::~BitReverseCTP() {}

u32 BitReverseCTP::nextDestination() {
  return dest_;
}

registerWithObjectFactory("BitReverse", ContinuousTrafficPattern,
                          BitReverseCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
