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
#include "util/DimensionIterator.h"

#include <cassert>

DimensionIterator::DimensionIterator(std::vector<u32> _widths)
    : widths_(_widths) {
  for (u32 i = 0; i < widths_.size(); i++) {
    assert(widths_[i] > 0);
  }
  reset();
}

DimensionIterator::~DimensionIterator() {}

bool DimensionIterator::next(std::vector<u32>* _address) {
  assert(_address->size() == widths_.size());
  bool retVal = more_;

  if (more_) {
    // copy address values
    *_address = state_;

    // advance state
    more_ = true;
    for (u32 d = 0; d < widths_.size(); d++) {
      if (state_.at(d) == (widths_.at(d) - 1)) {
        if (d == (widths_.size() - 1)) {
          more_ = false;
          break;
        } else {
          state_.at(d) = 0;
        }
      } else {
        state_.at(d)++;
        break;
      }
    }
  }
  return retVal;
}

void DimensionIterator::reset() {
  state_.clear();
  state_.resize(widths_.size(), 0);  // sets all to zero
  more_ = true;
}
