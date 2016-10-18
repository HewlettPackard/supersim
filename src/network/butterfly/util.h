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
#ifndef NETWORK_BUTTERFLY_UTIL_H_
#define NETWORK_BUTTERFLY_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace Butterfly {

void translateInterfaceIdToAddress(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    u32 _id, std::vector<u32>* _address);
u32 translateInterfaceAddressToId(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    const std::vector<u32>* _address);
void translateRouterIdToAddress(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    u32 _id, std::vector<u32>* _address);
u32 translateRouterAddressToId(
    u32 _routerRadix, u32 _numStages, u32 _stageWidth,
    const std::vector<u32>* _address);

}  // namespace Butterfly

#endif  // NETWORK_BUTTERFLY_UTIL_H_
