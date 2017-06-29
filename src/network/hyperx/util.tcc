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
#ifndef NETWORK_HYPERX_UTIL_H_
#error "don't include this file directly. use the .h file instead"
#endif  // NETWORK_HYPERX_UTIL_H_

#include <algorithm>
#include <tuple>
#include <unordered_set>

namespace HyperX {

template <typename T>
const T* uSetRandElement(const std::unordered_set<T>& uSet) {
  u64 randInd = gSim->rnd.nextU64(0, uSet.size() - 1);
  typename std::unordered_set<T>::const_iterator it = uSet.begin();
  std::advance(it, randInd);
  return &(*it);
}

template <typename T1, typename T2, typename T3>
bool tupleComp2(const std::tuple<T1, T2, T3>& tL,
                const std::tuple<T1, T2, T3>& tR) {
  if (std::get<2>(tL) == std::get<2>(tR)) {
    return tL < tR;
  } else {
    return std::get<2>(tL) < std::get<2>(tR);
  }
}

template <typename T1, typename T2, typename T3>
const std::tuple<T1, T2, T3>* uSetMinCong(
    const std::unordered_set<std::tuple<T1, T2, T3>>& uSet) {
  typename std::unordered_set<std::tuple<T1, T2, T3>>::const_iterator it =
      min_element(uSet.begin(), uSet.end(), tupleComp2<T1, T2, T3>);
  return &(*it);
}
}  // namespace HyperX
