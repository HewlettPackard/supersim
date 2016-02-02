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
#ifndef UTIL_DIMENSIONITERATOR_H_
#define UTIL_DIMENSIONITERATOR_H_

#include <prim/prim.h>

#include <vector>

class DimensionIterator {
 public:
  explicit DimensionIterator(std::vector<u32> _widths);
  ~DimensionIterator();
  bool next(std::vector<u32>* _address);
  void reset();

 private:
  std::vector<u32> widths_;
  std::vector<u32> state_;
  bool more_;
};

#endif  // UTIL_DIMENSIONITERATOR_H_
