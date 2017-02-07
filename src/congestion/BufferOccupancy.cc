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
#include "congestion/BufferOccupancy.h"

#include <cassert>
#include <cmath>

BufferOccupancy::BufferOccupancy(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : CongestionStatus(_name, _parent, _device, _settings),
      mode_(parseMode(_settings["mode"].asString())) {
  u32 totalVcs = numPorts_ * numVcs_;
  maximums_.resize(totalVcs, 0);
  counts_.resize(totalVcs, 0);
}

BufferOccupancy::~BufferOccupancy() {}

void BufferOccupancy::performInitCredits(u32 _port, u32 _vc, u32 _credits) {
  u32 vcIdx = device_->vcIndex(_port, _vc);
  maximums_.at(vcIdx) += _credits;
  counts_.at(vcIdx) += _credits;
  dbgprintf("max & count on %u is now %u", vcIdx, counts_.at(vcIdx));
}

void BufferOccupancy::performIncrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = device_->vcIndex(_port, _vc);
  dbgprintf("incr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) < maximums_.at(vcIdx));
  counts_.at(vcIdx)++;
}

void BufferOccupancy::performDecrementCredit(u32 _port, u32 _vc) {
  u32 vcIdx = device_->vcIndex(_port, _vc);
  dbgprintf("decr %u from %u", vcIdx, counts_.at(vcIdx));
  assert(counts_.at(vcIdx) > 0);
  counts_.at(vcIdx)--;
}

f64 BufferOccupancy::computeStatus(u32 _port, u32 _vc) const {
  switch (mode_) {
    case BufferOccupancy::Mode::kVc: {
      // return this VC's status
      u32 vcIdx = device_->vcIndex(_port, _vc);
      return ((f64)maximums_.at(vcIdx) - (f64)counts_.at(vcIdx)) /
          (f64)maximums_.at(vcIdx);
      break;
    }

    case BufferOccupancy::Mode::kPort: {
      // return the average status of all VCs in this port
      u32 curSum = 0;
      u32 maxSum = 0;
      for (u32 vc = 0; vc < numVcs_; vc++) {
        u32 vcIdx = device_->vcIndex(_port, vc);
        curSum += maximums_.at(vcIdx) - counts_.at(vcIdx);
        maxSum += maximums_.at(vcIdx);
      }
      return (f64)curSum / (f64)maxSum;
      break;
    }

    default:
      assert(false);
      break;
  }
}

BufferOccupancy::Mode BufferOccupancy::parseMode(const std::string& _mode) {
  if (_mode == "vc") {
    return BufferOccupancy::Mode::kVc;
  } else if (_mode == "port") {
    return  BufferOccupancy::Mode::kPort;
  } else {
    assert(false);
  }
}
