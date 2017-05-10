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
#ifndef UTIL_DIMENSIONALARRAY_H_
#define UTIL_DIMENSIONALARRAY_H_

#include <prim/prim.h>

#include <vector>

template <typename T>
class DimensionalArray {
 public:
  DimensionalArray();
  ~DimensionalArray();

  // Warning: this clears all data
  void setSize(const std::vector<u32>& _dimensionWidths);

  u32 size() const;
  u32 dimensionSize(u32 _dimension) const;

  u32 index(const std::vector<u32>& _index) const;
  T& at(const std::vector<u32>& _index);
  T& at(u32 _index);
  const T& at(u32 _index) const;

 private:
  std::vector<T> elements_;
  std::vector<u32> dimensionWidths_;
  std::vector<u32> dimensionScale_;
};

#include "util/DimensionalArray.tcc"

#endif  // UTIL_DIMENSIONALARRAY_H_
