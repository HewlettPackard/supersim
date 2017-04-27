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
#include "traffic/continuous/BitTransposeCFP.h"

#include <bits/bits.h>
#include <factory/Factory.h>

#include <cassert>

BitTransposeCFP::BitTransposeCFP(
    const std::string& _name, const Component* _parent,
    u32 _numTerminals, u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  assert(bits::isPow2(numTerminals_));

  u32 bitsNum = bits::ceilLog2(numTerminals_);
  assert(bitsNum % 2 == 0);
  u32 bitsNumHalf = bitsNum / 2;

  u32 left = self_ >> bitsNumHalf;
  u32 right = self_ & ((1 << bitsNumHalf) - 1);
  dest_ = (right << bitsNumHalf) | left;
}

BitTransposeCFP::~BitTransposeCFP() {}

u32 BitTransposeCFP::nextDestination() {
  return dest_;
}

registerWithFactory("bit_transpose", ContinuousTrafficPattern,
                    BitTransposeCFP, CONTINUOUSTRAFFICPATTERN_ARGS);
