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
#ifndef NETWORK_FATTREE_UTIL_H_
#define NETWORK_FATTREE_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace FatTree {

u32 leastCommonAncestor(const std::vector<u32>* _source,
                        const std::vector<u32>* _destination);
void translateInterfaceIdToAddress(
    u32 _numLevels, const std::vector<u32>& _terminalsPerGroup,
    u32 _id, std::vector<u32>* _address);
u32 translateInterfaceAddressToId(
    u32 _numLevels, const std::vector<u32>& _terminalsPerGroup,
    const std::vector<u32>* _address);
void translateRouterIdToAddress(
    u32 _numLevels, const std::vector<u32>& _routersPerRow,
    u32 _id, std::vector<u32>* _address);
u32 translateRouterAddressToId(
    u32 _numLevels, const std::vector<u32>& _routersPerRow,
    const std::vector<u32>* _address);
u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination);

}  // namespace FatTree

#endif  // NETWORK_FATTREE_UTIL_H_
