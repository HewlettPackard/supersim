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
#include "types/Credit.h"

#include <cassert>

Credit::Credit(u32 _nums) {
  assert(_nums > 0);
  numNums_ = _nums;
  putPos_ = 0;
  getPos_ = 0;
  nums_ = new u32[_nums];
}

Credit::~Credit() {
  delete[] nums_;
}

bool Credit::more() const {
  return getPos_ < putPos_;
}

void Credit::putNum(u32 _num) {
  assert(putPos_ < numNums_);
  nums_[putPos_++] = _num;
}

u32 Credit::getNum() {
  assert(getPos_ < numNums_);
  return nums_[getPos_++];
}
