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
#include "arbiter/Arbiter_TEST.h"

u32 hotCount(bool* _bools, u32 _len) {
  u32 cnt = 0;
  for (u32 idx = 0; idx < _len; idx++) {
    if (_bools[idx]) {
      cnt++;
    }
  }
  return cnt;
}

u32 winnerId(bool* _bools, u32 _len) {
  for (u32 idx = 0; idx < _len; idx++) {
    if (_bools[idx]) {
      return idx;
    }
  }
  return U32_MAX;
}
