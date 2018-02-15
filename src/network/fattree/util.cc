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
#include "network/fattree/util.h"

#include <strop/strop.h>

#include <cassert>
#include <cmath>

namespace FatTree {

u32 leastCommonAncestor(const std::vector<u32>* _source,
                        const std::vector<u32>* _destination) {
  assert(_source->size() == _destination->size());
  for (s32 level = _source->size() - 1; level >= 0; level--) {
    if (_source->at(level) != _destination->at(level)) {
      return level;
    }
  }
  return 0;
}

void translateInterfaceIdToAddress(
    u32 _numLevels, const std::vector<u32>& _terminalsPerGroup,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(_numLevels);
  // work in reverse for little endian format
  for (s32 level = _numLevels - 1; level >= 0; level--) {
    if (level > 0) {
      u32 subGroupSize = _terminalsPerGroup.at(level-1);
      _address->at(level) = _id / subGroupSize;
      _id %= subGroupSize;
    } else {
      _address->at(level) = _id;
    }
  }
}

u32 translateInterfaceAddressToId(
    u32 _numLevels, const std::vector<u32>& _terminalsPerGroup,
    const std::vector<u32>* _address) {
  u32 sum = 0;
  for (s32 level = _numLevels - 1; level >= 0; level--) {
    if (level > 0) {
      u32 subGroupSize = _terminalsPerGroup.at(level-1);
      sum += _address->at(level) * subGroupSize;
    } else {
      sum += _address->at(level);
    }
  }
  return sum;
}

void translateRouterIdToAddress(
    u32 _numLevels, const std::vector<u32>& _routersPerRow,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(2);
  for (u32 l = 0; l < _numLevels; l++) {
    u32 rowRouters = _routersPerRow.at(l);
    if (_id < rowRouters) {
      _address->at(0) = l;
      _address->at(1) = _id;
      break;
    } else {
      _id -= rowRouters;
    }
  }
}

u32 translateRouterAddressToId(
    u32 _numLevels, const std::vector<u32>& _routersPerRow,
    const std::vector<u32>* _address) {
  u32 level = _address->at(0);
  u32 prev = 0;
  for (u32 l = 0; l < level; l++) {
    prev += _routersPerRow.at(l);
  }
  return prev + _address->at(1);
}

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination) {
  u32 travLevels;
  assert(_source->size() == _destination->size());
  for (travLevels = _source->size(); travLevels > 0; travLevels--) {
    if (_source->at(travLevels-1) != _destination->at(travLevels-1) ||
        travLevels == 1) {
      break;
    }
  }
  return travLevels * 2 - 1;
}

}  // namespace FatTree
