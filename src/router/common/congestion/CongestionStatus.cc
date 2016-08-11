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
#include "router/common/congestion/CongestionStatus.h"

#include <cassert>
#include <cmath>

#include "router/Router.h"

CongestionStatus::CongestionStatus(
    const std::string& _name, const Component* _parent, Router* _router,
    Json::Value _settings)
    : Component(_name, _parent), router_(_router),
      numPorts_(router_->numPorts()), numVcs_(router_->numVcs()),
      latency_(_settings["latency"].asUInt()),
      granularity_(_settings["granularity"].asUInt()) {
  assert(latency_ > 0);
  assert(!_settings["granularity"].isNull());
}

CongestionStatus::~CongestionStatus() {}

void CongestionStatus::initCredits(u32 _port, u32 _vc, u32 _credits) {
  assert(_port < numPorts_);
  assert(_vc < numVcs_);
  assert(_credits > 0);
  performInitCredits(_port, _vc, _credits);
}

void CongestionStatus::incrementCredit(u32 _port, u32 _vc) {
  createEvent(_port, _vc, INCR);
}

void CongestionStatus::decrementCredit(u32 _port, u32 _vc) {
  createEvent(_port, _vc, DECR);
}

f64 CongestionStatus::status(u32 _port, u32 _vc) const {
  assert(gSim->epsilon() == 0);
  f64 value = computeStatus(_port, _vc);
  assert(value >= 0.0);
  assert(value <= 1.0);
  if (granularity_ > 0) {
    value = round(value * granularity_) / granularity_;
  }
  return value;
}

void CongestionStatus::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() > 0);
  u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
  u32 port, vc;
  router_->vcIndexInv(vcIdx, &port, &vc);
  switch (_type) {
    case CongestionStatus::INCR:
      performIncrementCredit(port, vc);
      break;
    case CongestionStatus::DECR:
      performDecrementCredit(port, vc);
      break;
    default:
      assert(false);
  }
}

void CongestionStatus::createEvent(u32 _port, u32 _vc, s32 _type) {
  assert(gSim->epsilon() > 0);
  assert(_port < numPorts_);
  assert(_vc < numVcs_);
  u64 time = latency_ == 1 ? gSim->time() : gSim->futureCycle(latency_ - 1);
  u32 vcIdx = router_->vcIndex(_port, _vc);
  addEvent(time, gSim->epsilon() + 1, reinterpret_cast<void*>(vcIdx), _type);
}
