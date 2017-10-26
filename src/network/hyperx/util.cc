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
#include "network/hyperx/util.h"

#include <cassert>

namespace HyperX {

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _dimensions) {
  u32 minHops = 1;
  for (u32 dim = 0; dim < _dimensions; dim++) {
    if (_source->at(dim+1) != _destination->at(dim+1)) {
      minHops += 1;
    }
  }
  return minHops;
}

}  // namespace HyperX
