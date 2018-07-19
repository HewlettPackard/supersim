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
#include "network/torus/util.h"
#include <algorithm>
#include <cassert>

namespace Torus {

u32 computeInputPortDim(const std::vector<u32>& _dimensionWidths,
                        const std::vector<u32>& _dimensionWeights,
                        u32 _concentration, u32 _inputPort) {
  // determine which network dimension this port is attached to
  if (_inputPort < _concentration) {
    return U32_MAX;  // terminal dimension
  }

  u32 portBase = _concentration;
  for (u32 dim = 0; dim < _dimensionWidths.size(); dim++) {
    u32 dimWeight = _dimensionWeights.at(dim);
    if ((_inputPort >= portBase) && (_inputPort < (portBase + 2 * dimWeight))) {
      return dim;
    }
    portBase += 2 * dimWeight;
  }
  assert(false);
}

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _dimensions,
                       const std::vector<u32>& _dimensionWidths) {
  u32 minHops = 1;
  for (u32 dim = 0; dim < _dimensions; dim++) {
    u32 src = _source->at(dim+1);
    u32 dst = _destination->at(dim+1);
    if (src != dst) {
      u32 rightDelta = ((dst > src) ?
                        (dst - src) :
                        (dst + _dimensionWidths.at(dim) - src));
      u32 leftDelta = ((src > dst) ?
                       (src - dst) :
                       (src + _dimensionWidths.at(dim) - dst));
      minHops += std::min(rightDelta, leftDelta);
    }
  }
  return minHops;
}
}  // namespace Torus
