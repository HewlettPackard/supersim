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
#include "router/common/congestion/PhantomBufferOccupancy.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include "router/Router.h"

const s32 PHANTOM = 0x87;

PhantomBufferOccupancy::PhantomBufferOccupancy(
    const std::string& _name, const Component* _parent, Router* _router,
    Json::Value _settings)
    : CongestionStatus(_name, _parent, _router, _settings),
      valueCoeff_(_settings["value_coeff"].asDouble()),
      lengthCoeff_(_settings["length_coeff"].asDouble()) {
  assert(!_settings["value_coeff"].isNull());
  assert(!_settings["length_coeff"].isNull());

  u32 totalVcs = numPorts_ * numVcs_;
  maximums_.resize(totalVcs);
  counts_.resize(totalVcs);
  windows_.resize(totalVcs, 0);
}

PhantomBufferOccupancy::~PhantomBufferOccupancy() {}

void PhantomBufferOccupancy::processEvent(void* _event, s32 _type) {
  if (_type == PHANTOM) {
    // decrement the phantom congestion window
    u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
    dbgprintf("-window %u from %u", vcIdx, windows_.at(vcIdx));
    windows_.at(vcIdx)--;
  } else {
    // pass event to superclass
    CongestionStatus::processEvent(_event, _type);
  }
}

void PhantomBufferOccupancy::performInitCredits(
    u32 _port, u32 _vc, u32 _credits) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  maximums_.at(vcIdx) = _credits;
  counts_.at(vcIdx) = _credits;
}

void PhantomBufferOccupancy::performIncrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  dbgprintf("incr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) < maximums_.at(vcIdx));
  counts_.at(vcIdx)++;
}

void PhantomBufferOccupancy::performDecrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  dbgprintf("decr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) > 0);
  counts_.at(vcIdx)--;

  // push credit into phantom congestion detection window
  // set an event to decrement the phantom congestion window
  dbgprintf("+window %u from %u", vcIdx, windows_.at(vcIdx));
  windows_.at(vcIdx)++;
  Channel* ch = router_->getOutputChannel(_port);
  u32 windowLength = (u32)(ch->latency() * lengthCoeff_);
  u64 time = gSim->futureCycle(windowLength);
  addEvent(time, gSim->epsilon(), reinterpret_cast<void*>(vcIdx), PHANTOM);
}

f64 PhantomBufferOccupancy::computeStatus(u32 _port, u32 _vc) const {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  dbgprintf("q=%u w=%u m=%u vm=%f", counts_.at(vcIdx), windows_.at(vcIdx),
            maximums_.at(vcIdx), valueCoeff_);
  f64 status = (((f64)maximums_.at(vcIdx) - (f64)counts_.at(vcIdx) -
                 (f64)windows_.at(vcIdx) * valueCoeff_) /
                (f64)maximums_.at(vcIdx));
  dbgprintf("sts=%f", status);
  return std::min(1.0, std::max(0.0, status));
}
