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
#include "network/butterfly/util.h"

#include <cassert>
#include <cmath>

namespace Butterfly {

void translateInterfaceIdToAddress(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(_numStages);
  // work in reverse for little endian format
  for (u32 exp = 0, row = _numStages - 1; exp < _numStages; exp++, row--) {
    u32 divisor = (u32)pow(_routerRadix, row);
    _address->at(exp) = _id / divisor;
    _id %= divisor;
  }
}

u32 translateInterfaceAddressToId(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    const std::vector<u32>* _address) {
  u32 sum = 0;
  u32 pow = 1;
  for (u32 stage = 0; stage < _numStages; stage++) {
    u32 index = _numStages - 1 - stage;
    sum += _address->at(index) * pow;
    pow *= _routerRadix;
  }
  return sum;
}

void translateRouterIdToAddress(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(2);
  _address->at(0) = _id / _stageWidth;
  _address->at(1) = _id % _stageWidth;
}

u32 translateRouterAddressToId(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    const std::vector<u32>* _address) {
  return _address->at(0) * _stageWidth + _address->at(1);
}

u32 computeMinimalHops(u32 _numStages) {
  return _numStages - 1 + 2;
}
}  // namespace Butterfly
