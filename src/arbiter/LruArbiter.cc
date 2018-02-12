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
#include "arbiter/LruArbiter.h"

#include <factory/ObjectFactory.h>

#include <vector>

LruArbiter::LruArbiter(const std::string& _name, const Component* _parent,
                       u32 _size, Json::Value _settings)
    : Arbiter(_name, _parent, _size, _settings) {
  // create a random ordered priority list
  std::vector<u32> clients(size_);
  for (u32 idx = 0; idx < size_; idx++) {
    clients.at(idx) = idx;
  }
  gSim->rnd.shuffle(&clients);
  for (auto client : clients) {
    priority_.push_back(client);
  }

  // artifically set the last winner
  lastWinner_ = priority_.end();
}

LruArbiter::~LruArbiter() {}

void LruArbiter::latch() {
  u32 value = *lastWinner_;
  priority_.erase(lastWinner_);
  priority_.push_back(value);
}

u32 LruArbiter::arbitrate() {
  u32 winner = U32_MAX;
  for (auto it = priority_.begin(); it != priority_.end(); ++it) {
    u32 client = *it;
    if (*requests_[client]) {
      winner = client;
      *grants_[client] = true;
      lastWinner_ = it;
      break;
    }
  }
  return winner;
}

registerWithObjectFactory("lru", Arbiter,
                          LruArbiter, ARBITER_ARGS);
