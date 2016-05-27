/*
 * Copyright 2016 Ashish Chaudhari, Franky Romero, Nehal Bhandari, Wasam Altoyan
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
#ifndef NETWORK_SLIMFLY_UTIL_H_
#define NETWORK_SLIMFLY_UTIL_H_

#include <prim/prim.h>
#include <vector>

namespace SlimFly {

bool isPrime(u32 _width);
u32 createGeneratorSet(
  u32 _width, int delta, std::vector<u32>* X, std::vector<u32>* X_i);
void addressFromInterfaceId(
  u32 _id, u32 _width, u32 _concentration, std::vector<u32>* _address);
u32 ifaceIdFromAddress(
    const std::vector<u32>& _address, u32 _width, u32 _concentration);

}  // namespace SlimFly
#endif  // NETWORK_SLIMFLY_UTIL_H_
