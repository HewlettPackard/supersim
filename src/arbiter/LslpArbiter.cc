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
#include "arbiter/LslpArbiter.h"

#include <factory/Factory.h>

LslpArbiter::LslpArbiter(const std::string& _name, const Component* _parent,
                         u32 _size, Json::Value _settings)
    : Arbiter(_name, _parent, _size, _settings) {
  nextPriority_ = gSim->rnd.nextU64(0, size_ - 1);
  latch();
}

LslpArbiter::~LslpArbiter() {}

void LslpArbiter::latch() {
  priority_ = nextPriority_;
}

u32 LslpArbiter::arbitrate() {
  u32 winner = U32_MAX;
  for (u32 idx = priority_; idx < (size_ + priority_); idx++) {
    u32 client = idx % size_;
    if (*requests_[client]) {
      winner = client;
      *grants_[client] = true;
      nextPriority_ = (winner + 1) % size_;
      break;
    }
  }
  return winner;
}

registerWithFactory("lslp", Arbiter,
                    LslpArbiter, ARBITER_ARGS);
