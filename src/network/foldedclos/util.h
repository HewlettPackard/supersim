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
#ifndef NETWORK_FOLDEDCLOS_UTIL_H_
#define NETWORK_FOLDEDCLOS_UTIL_H_

#include <prim/prim.h>

#include <vector>

namespace FoldedClos {

void translateInterfaceIdToAddress(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    u32 _id, std::vector<u32>* _address);
u32 translateInterfaceAddressToId(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    const std::vector<u32>* _address);
void translateRouterIdToAddress(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    u32 _id, std::vector<u32>* _address);
u32 translateRouterAddressToId(
    u32 _halfRadix, u32 _numLevels, u32 _rowRouters,
    const std::vector<u32>* _address);

}  // namespace FoldedClos

#endif  // NETWORK_FOLDEDCLOS_UTIL_H_
