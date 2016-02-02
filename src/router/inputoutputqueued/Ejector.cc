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
#include "router/inputoutputqueued/Ejector.h"

#include <cassert>

#include <string>

#include "router/inputoutputqueued/Router.h"

namespace InputOutputQueued {

Ejector::Ejector(
    std::string _name, Router* _router, u32 _portId)
    : Component(_name, _router),
      router_(_router), portId_(_portId) {
  lastSetTime_ = U32_MAX;
}

Ejector::~Ejector() {}

void Ejector::receiveFlit(u32 _port, Flit* _flit) {
  // verify one flit per cycle
  u64 nextTime = gSim->time();
  assert((lastSetTime_ != nextTime) || (lastSetTime_ == U32_MAX));
  lastSetTime_ = nextTime;

  // send flit using the router
  router_->sendFlit(portId_, _flit);
}

}  // namespace InputOutputQueued
