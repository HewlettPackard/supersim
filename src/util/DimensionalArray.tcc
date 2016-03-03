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
#include <stdexcept>
#include <vector>

template <typename T>
DimensionalArray<T>::DimensionalArray() {}

template <typename T>
DimensionalArray<T>::~DimensionalArray() {}

template <typename T>
void DimensionalArray<T>::setSize(const std::vector<u32>& _dimensionWidths) {
  // determine the total number of element
  u32 totalSize = 0;
  if (_dimensionWidths.size() > 0) {
    totalSize++;
    for (auto it = _dimensionWidths.begin(); it != _dimensionWidths.end();
         ++it) {
      totalSize *= *it;
    }
  }

  // clear and resize
  elements_.clear();
  elements_.resize(totalSize);

  // copy dimension widths
  dimensionWidths_ = _dimensionWidths;

  // set the dimension scale
  dimensionScale_.resize(dimensionWidths_.size());
  u32 scale = totalSize;
  for (s32 i = dimensionWidths_.size() - 1; i >= 0; i--) {
    scale /= dimensionWidths_.at(i);
    dimensionScale_.at(i) = scale;
  }
}

template <typename T>
u32 DimensionalArray<T>::size() const {
  return elements_.size();
}

template <typename T>
u32 DimensionalArray<T>::dimensionSize(u32 _dimension) const {
  return dimensionWidths_.at(_dimension);
}

template <typename T>
u32 DimensionalArray<T>::index(const std::vector<u32>& _index) const {
  if (elements_.size() == 0) {
    throw std::out_of_range("there are no elements in the DimensionalArray");
  }
  if (_index.size() != dimensionWidths_.size()) {
    throw std::out_of_range("'index' size must match number of dimensions");
  }

  u32 element = 0;
  for (s32 i = _index.size() - 1; i >= 0; --i) {
    if (_index.at(i) >= dimensionWidths_.at(i)) {
      throw std::out_of_range("'index' out of bounds");
    }
    u32 dimScale = dimensionScale_.at(i);
    u32 dimIndex = _index.at(i);
    element += dimScale * dimIndex;
  }
  return element;
}

template <typename T>
T& DimensionalArray<T>::at(const std::vector<u32>& _index) {
  return elements_.at(index(_index));
}

template <typename T>
T& DimensionalArray<T>::at(u32 _index) {
  return elements_.at(_index);
}

template <typename T>
const T& DimensionalArray<T>::at(u32 _index) const {
  return elements_.at(_index);
}

/*
  18
  z - 111111222222333333 - 3 - 6
  y - 111222111222111222 - 2 - 3
  x - 123123123123123123 - 3 - 1
*/
