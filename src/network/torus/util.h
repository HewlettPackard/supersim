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
#ifndef NETWORK_TORUS_UTIL_H_
#define NETWORK_TORUS_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace Torus {

// This function determines the dimension correspondance of an input port.
//  This returns U32_MAX for terminal ports.
u32 computeInputPortDim(const std::vector<u32>& _dimensionWidths,
                        const std::vector<u32>& _dimensionWeights,
                        u32 _concentration, u32 _inputPort);

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _dimensions,
                       const std::vector<u32>& _dimensionWidths);
}  // namespace Torus

#endif  // NETWORK_TORUS_UTIL_H_
