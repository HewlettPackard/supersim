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
#ifndef TYPES_CREDIT_H_
#define TYPES_CREDIT_H_

#include <prim/prim.h>

class Credit {
 public:
  explicit Credit(u32 _nums);
  ~Credit();
  bool more() const;
  void putNum(u32 _num);
  u32 getNum();

 private:
  u32 numNums_;
  u32 putPos_;
  u32 getPos_;
  u32* nums_;
};

#endif  // TYPES_CREDIT_H_
