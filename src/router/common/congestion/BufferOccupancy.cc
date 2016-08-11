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
#include "router/common/congestion/BufferOccupancy.h"

#include <cassert>
#include <cmath>

#include "router/Router.h"

BufferOccupancy::BufferOccupancy(
    const std::string& _name, const Component* _parent, Router* _router,
    Json::Value _settings)
    : CongestionStatus(_name, _parent, _router, _settings) {
  u32 totalVcs = numPorts_ * numVcs_;
  maximums_.resize(totalVcs);
  counts_.resize(totalVcs);
}

BufferOccupancy::~BufferOccupancy() {}

void BufferOccupancy::performInitCredits(u32 _port, u32 _vc, u32 _credits) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  maximums_.at(vcIdx) = _credits;
  counts_.at(vcIdx) = _credits;
}

void BufferOccupancy::performIncrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  dbgprintf("incr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) < maximums_.at(vcIdx));
  counts_.at(vcIdx)++;
}

void BufferOccupancy::performDecrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  dbgprintf("decr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) > 0);
  counts_.at(vcIdx)--;
}

f64 BufferOccupancy::computeStatus(u32 _port, u32 _vc) const {
  u32 vcIdx = router_->vcIndex(_port, _vc);
  return ((f64)maximums_.at(vcIdx) - (f64)counts_.at(vcIdx)) /
      (f64)maximums_.at(vcIdx);
}
