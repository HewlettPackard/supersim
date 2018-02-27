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
#ifndef NETWORK_DRAGONFLY_UTIL_H_
#define NETWORK_DRAGONFLY_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace Dragonfly {
u32 computeOffset(u32 _source, u32 _destination, u32 _width);
u32 computeLocalSrcPort(u32 _portBase, u32 _offset, u32 _localWeight,
                        u32 _weight);
u32 computeLocalDstPort(u32 _portBase, u32 _offset, u32 _localWidth,
                        u32 _localWeight, u32 _weight);
u32 computeGlobalPort(u32 _globalWidth, u32 _oneGlobalWeight,
                      u32 _globalOffset);
void computeGlobalToRouterMap(u32 _routerGlobalPortBase,
                              u32 _globalPortsPerRouter,
                              u32 _globalWidth, u32 _globalWeight,
                              u32 _localWidth,
                              u32 _thisGlobalWeight, u32 _thisGlobalOffset,
                              u32* _globalPort, u32* _localRouter,
                              u32* _localPort);
void translateInterfaceIdToAddress(
    u32 _concentration, u32 _localWidth,
    u32 _id, std::vector<u32>* _address);
u32 translateInterfaceAddressToId(
    u32 _concentration, u32 _localWidth,
    const std::vector<u32>* _address);

void translateRouterIdToAddress(
    u32 _localWidth, u32 _id, std::vector<u32>* _address);
u32 translateRouterAddressToId(
    u32 _localWidth, const std::vector<u32>* _address);

u32 computeMinimalHops(const std::vector<u32>* _source,
                       const std::vector<u32>* _destination,
                       u32 _globalWidth, u32 _globalWeight,
                       u32 _routerGlobalPortBase,
                       u32 _globalPortsPerRouter,
                       u32 _localWidth);
}  // namespace Dragonfly

#endif  // NETWORK_DRAGONFLY_UTIL_H_
