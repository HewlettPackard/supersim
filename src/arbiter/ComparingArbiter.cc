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
#include "arbiter/ComparingArbiter.h"

#include <factory/Factory.h>

ComparingArbiter::ComparingArbiter(
    const std::string& _name, const Component* _parent,
    u32 _size, Json::Value _settings)
    : Arbiter(_name, _parent, _size, _settings) {
  greater_ = _settings["greater"].asBool();
  temp_.reserve(size_);
}

ComparingArbiter::~ComparingArbiter() {}

u32 ComparingArbiter::arbitrate() {
  bool one = false;
  u64 best = U64_MAX;
  for (u32 client = 0; client < size_; client++) {
    if (*requests_[client]) {
      u64 cmeta = *metadatas_[client];
      if (!one) {
        // handle the first one
        one = true;
        best = cmeta;
        temp_.push_back(client);
      } else {
        // handle remaining
        if (((greater_) && (cmeta > best)) ||
            ((!greater_) && (cmeta < best))) {
          // new best
          best = cmeta;
          temp_.clear();
          temp_.push_back(client);
        } else if (cmeta == best) {
          // match best
          temp_.push_back(client);
        }
      }
    }
  }

  // randomly choose winner from compared best set
  u32 winner = U32_MAX;
  if (temp_.size() > 0) {
    u32 idx = gSim->rnd.nextU64(0, temp_.size() - 1);
    winner = temp_.at(idx);
    *grants_[winner] = true;
  }
  temp_.clear();
  return winner;
}

registerWithFactory("comparing", Arbiter,
                    ComparingArbiter, ARBITER_ARGS);
