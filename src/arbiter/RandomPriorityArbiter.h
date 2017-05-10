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
#ifndef ARBITER_RANDOMPRIORITYARBITER_H_
#define ARBITER_RANDOMPRIORITYARBITER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "arbiter/Arbiter.h"
#include "event/Component.h"

// chooses random priority
class RandomPriorityArbiter : public Arbiter {
 public:
  RandomPriorityArbiter(const std::string& _name, const Component* _parent,
                        u32 _size, Json::Value _settings);
  ~RandomPriorityArbiter();

  u32 arbitrate() override;
};


#endif  // ARBITER_RANDOMPRIORITYARBITER_H_
