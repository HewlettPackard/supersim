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
#ifndef ARBITER_COMPARINGARBITER_H_
#define ARBITER_COMPARINGARBITER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "event/Component.h"

// compares the metadata value, random tie-breaker
class ComparingArbiter : public Arbiter {
 public:
  ComparingArbiter(const std::string& _name, const Component* _parent,
                   u32 _size, Json::Value _settings);
  ~ComparingArbiter();

  u32 arbitrate() override;

 private:
  bool greater_;
};


#endif  // ARBITER_COMPARINGARBITER_H_
