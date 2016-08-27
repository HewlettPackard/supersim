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
#include "arbiter/RandomPriorityArbiter.h"

RandomPriorityArbiter::RandomPriorityArbiter(
    const std::string& _name, const Component* _parent, u32 _size,
    Json::Value _settings)
    : Arbiter(_name, _parent, _size) {}

RandomPriorityArbiter::~RandomPriorityArbiter() {}

u32 RandomPriorityArbiter::arbitrate() {
  u32 winner = U32_MAX;
  u32 offset = gSim->rnd.nextU64(0, size_ - 1);
  for (u32 idx = 0; idx < size_; idx++) {
    u32 client = (idx + offset) % size_;
    if (*requests_[client]) {
      winner = client;
      *grants_[winner] = true;
      break;
    }
  }
  return winner;
}
