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
#ifndef TYPES_CREDITSENDER_H_
#define TYPES_CREDITSENDER_H_

#include <prim/prim.h>

class CreditSender {
 public:
  CreditSender();
  virtual ~CreditSender();
  virtual void sendCredit(u32 _port, u32 _vc) = 0;
};

#endif  // TYPES_CREDITSENDER_H_
