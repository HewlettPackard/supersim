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
#include "arbiter/LslpArbiter.h"

LslpArbiter::LslpArbiter(const std::string& _name, const Component* _parent,
                         u32 _size, Json::Value _settings)
    : Arbiter(_name, _parent, _size) {
  prevPriority_ = static_cast<u32>(gSim->rnd.nextU64(0, size_));
  latch();
}

LslpArbiter::~LslpArbiter() {}

void LslpArbiter::latch() {
  priority_ = prevPriority_;
}

u32 LslpArbiter::arbitrate() {
  u32 winner = U32_MAX;
  for (u32 idx = priority_; idx < (size_ + priority_); idx++) {
    u32 client = idx % size_;
    if (*requests_[client]) {  // requesting
      winner = client;
      *grants_[client] = true;
      break;
    }
  }
  if (winner != U32_MAX) {
    prevPriority_ = (winner + 1) % size_;
  } else {
    prevPriority_ = priority_;
  }
  return winner;
}
