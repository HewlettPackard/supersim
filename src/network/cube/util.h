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
#ifndef NETWORK_CUBE_UTIL_H_
#define NETWORK_CUBE_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace Cube {

void translateInterfaceIdToAddress(
    u32 _id, const std::vector<u32>& _widths, u32 _concentration,
    std::vector<u32>* _address);

u32 translateInterfaceAddressToId(
    const std::vector<u32>* _address, const std::vector<u32>& _widths,
    u32 _concentration);

void translateRouterIdToAddress(
    const u32 _id, const std::vector<u32>& _widths, std::vector<u32>* _address);

u32 translateRouterAddressToId(
    const std::vector<u32>* _address, const std::vector<u32>& _widths);

}  // namespace Cube

#endif  // NETWORK_CUBE_UTIL_H_
