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
#include "network/dragonfly/util.h"

#include <strop/strop.h>

#include <cassert>
#include <cmath>

namespace Dragonfly {

u32 computeOffset(u32 _source, u32 _destination, u32 _width) {
  u32 offset;
  if (_destination < _source) {
    offset = (_destination + _width) - _source;
  } else {
    offset = _destination - _source;
  }
  return offset;
}

u32 computeLocalSrcPort(u32 _portBase, u32 _offset, u32 _localWeight,
                        u32 _weight) {
  // compure src port to local router  for given offset
  return _portBase + ((_offset - 1) * _localWeight) + _weight;
}

u32 computeLocalDstPort(u32 _portBase, u32 _offset, u32 _localWidth,
                        u32 _localWeight, u32 _weight) {
  // compure dst port of local router for given offset
  return _portBase + ((_localWidth - 1) * _localWeight) -
      (_offset * _localWeight) + _weight;
}

void computeGlobalToRouterMap(u32 _routerGlobalPortBase,
                              u32 _globalPortsPerRouter,
                              u32 _globalWidth, u32 _globalWeight,
                              u32 _localWidth,
                              u32 _thisGlobalWeight, u32 _thisGlobalOffset,
                              u32* _globalPort, u32* _localRouter,
                              u32* _localPort) {
  // compute router and port connected to a given group global port
  assert(_globalPort);
  *_globalPort = ((_thisGlobalWeight * (_globalWidth - 1)) +
                  (_thisGlobalOffset - 1));

  if (_localRouter) {
    *_localRouter = *_globalPort / _globalPortsPerRouter;
  }
  if (_localPort) {
    *_localPort = ((*_globalPort % _globalPortsPerRouter)
                   + _routerGlobalPortBase);
  }
}

void translateInterfaceIdToAddress(
    u32 _concentration, u32 _localWidth,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(3);
  u32 group = _id / (_localWidth * _concentration);
  _id %= _localWidth * _concentration;

  u32 router = _id / _concentration;
  _id %= _concentration;

  _address->at(0) = _id;
  _address->at(1) = router;
  _address->at(2) = group;
}

u32 translateInterfaceAddressToId(
    u32 _concentration, u32 _localWidth,
    const std::vector<u32>* _address) {
  u32 c = _address->at(0);
  u32 r = _address->at(1);
  u32 g = _address->at(2);

  u32 gBase = g * (_localWidth * _concentration);
  u32 rBase = r * _concentration;
  return gBase + rBase + c;
}

void translateRouterIdToAddress(
    u32 _localWidth,
    u32 _id, std::vector<u32>* _address) {
  _address->resize(2);
  _address->at(0) = _id % _localWidth;  // router
  _address->at(1) = _id / _localWidth;  // group
}

u32 translateRouterAddressToId(
    u32 _localWidth,
    const std::vector<u32>* _address) {
  u32 r = _address->at(0);
  u32 group = _address->at(1);
  u32 base = group * _localWidth;
  return base + r;
}

u32 computeMinimalHops(
    const std::vector<u32>* _source, const std::vector<u32>* _destination,
    u32 _globalWidth, u32 _globalWeight, u32 _routerGlobalPortBase,
    u32 _globalPortsPerRouter,
    u32 _localWidth) {
  assert(_source->size() == 3);
  assert(_destination->size() == 3);
  // same group
  if (_source->at(2) == _destination->at(2)) {
    if (_source->at(1) == _destination->at(1)) {
      return 1;
    } else {
      return 2;
    }
  } else {
    // different groups
    u32 forwardOffset = computeOffset(_source->at(2), _destination->at(2),
                                      _globalWidth);
    u32 reverseOffset = _globalWidth - forwardOffset;
    u32 minHops = U32_MAX;
    for (u32 weight = 0; weight < _globalWeight; weight++) {
      u32 srcGlobalPort;
      u32 dstGlobalPort;

      // router to exit src group
      u32 srcGroupGlobalRouter;
      computeGlobalToRouterMap(_routerGlobalPortBase,
                               _globalPortsPerRouter,
                               _globalWidth, _globalWeight, _localWidth,
                               weight, forwardOffset, &srcGlobalPort,
                               &srcGroupGlobalRouter, nullptr);
      // router to enter dst group
      u32 dstGroupGlobalRouter;
      computeGlobalToRouterMap(_routerGlobalPortBase,
                               _globalPortsPerRouter,
                               _globalWidth, _globalWeight, _localWidth,
                               weight, reverseOffset, &dstGlobalPort,
                               &dstGroupGlobalRouter, nullptr);

      u32 hops = 2;
      if (_source->at(1) != srcGroupGlobalRouter) {
        // src router is not exit router
        hops++;
      }
      if (_destination->at(1) != dstGroupGlobalRouter) {
        // dst is not at entering router
        hops++;
      }
      if (hops < minHops) {
        minHops = hops;
      }
    }
    assert(minHops != U32_MAX);
    return minHops;
  }
}
}  // namespace Dragonfly
