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
#include "network/foldedclos/util.h"

#include <cmath>

namespace FoldedClos {

void translateInterfaceIdToAddress(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(_numLevels);
  // work in reverse for little endian format
  for (u32 exp = 0, row = _numLevels - 1; exp < _numLevels; exp++, row--) {
    u32 divisor = (u32)pow(_halfRadix, row);
    _address->at(row) = _id / divisor;
    _id %= divisor;
  }
}

u32 translateInterfaceAddressToId(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    const std::vector<u32>* _address) {
  u32 sum = 0;
  // work in reverse for little endian format
  for (u32 exp = 0, row = _numLevels - 1; exp < _numLevels; exp++, row--) {
    u32 multiplier = (u32)pow(_halfRadix, row);
    sum += _address->at(row) * multiplier;
  }
  return sum;
}

void translateRouterIdToAddress(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(2);
  _address->at(0) = _id / _rowRouters;
  _address->at(1) = _id % _rowRouters;
}

u32 translateRouterAddressToId(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    const std::vector<u32>* _address) {
  return _address->at(0) * _rowRouters + _address->at(1);
}

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _numLevels) {
  u32 travLevels;
  for (travLevels = _numLevels; travLevels > 0; travLevels--) {
    if (_source->at(travLevels-1) != _destination->at(travLevels-1) ||
        travLevels == 1) {
      break;
    }
  }
  return travLevels * 2 - 1;
}

}  // namespace FoldedClos
