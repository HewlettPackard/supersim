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
#include "arbiter/RandomArbiter.h"

#include <factory/ObjectFactory.h>

RandomArbiter::RandomArbiter(
    const std::string& _name, const Component* _parent, u32 _size,
    Json::Value _settings)
    : Arbiter(_name, _parent, _size, _settings) {
  temp_.reserve(size_);
}

RandomArbiter::~RandomArbiter() {}

u32 RandomArbiter::arbitrate() {
  for (u32 client = 0; client < size_; client++) {
    if (*requests_[client]) {
      temp_.push_back(client);
    }
  }
  u32 winner = U32_MAX;
  if (temp_.size() > 0) {
    u32 idx = gSim->rnd.nextU64(0, temp_.size() - 1);
    winner = temp_.at(idx);
    *grants_[winner] = true;
  }
  temp_.clear();
  return winner;
}

registerWithObjectFactory("random", Arbiter,
                          RandomArbiter, ARBITER_ARGS);
