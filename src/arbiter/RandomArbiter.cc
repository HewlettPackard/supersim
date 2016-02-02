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
#include "arbiter/RandomArbiter.h"

RandomArbiter::RandomArbiter(const std::string& _name, const Component* _parent,
                             u32 _size, Json::Value _settings)
    : Arbiter(_name, _parent, _size) {}

RandomArbiter::~RandomArbiter() {}

u32 RandomArbiter::arbitrate() {
  u32 winner = U32_MAX;
  for (u32 client = 0; client < size_; client++) {
    if (*requests_[client]) {  // requesting
      // input enabled
      if (winner == U32_MAX) {
        // first contender
        winner = client;
      } else if (gSim->rnd.nextBool()) {
        // won random tie-breaker
        winner = client;
      }
    }
  }
  if (winner != U32_MAX) {
    *grants_[winner] = true;
  }
  return winner;
}
