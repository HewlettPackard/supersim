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
#ifndef TYPES_FLITSENDER_H_
#define TYPES_FLITSENDER_H_

#include <prim/prim.h>

#include "types/Flit.h"

class FlitSender {
 public:
  FlitSender();
  virtual ~FlitSender();
  virtual void sendFlit(u32 _port, Flit* _flit) = 0;
};

#endif  // TYPES_FLITSENDER_H_
