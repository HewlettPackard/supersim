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
#include "congestion/BufferOccupancy.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include <algorithm>

namespace {
const s32 INCR = 0x50;
const s32 DECR = 0xAF;
const s32 PHANTOM = 0x87;
}  // namespace

BufferOccupancy::BufferOccupancy(
    const std::string& _name, const Component* _parent, PortedDevice* _device,
    Json::Value _settings)
    : CongestionStatus(_name, _parent, _device, _settings),
      latency_(_settings["latency"].asUInt()),
      mode_(parseMode(_settings["mode"].asString())) {
  assert(latency_ > 0);
  u32 totalVcs = numPorts_ * numVcs_;
  creditMaximums_.resize(totalVcs, 0);
  creditCounts_.resize(totalVcs, 0);
  flitsOutstanding_.resize(totalVcs, 0);

  // phantom is an optional setting
  phantom_ = false;
  if (_settings.isMember("phantom")) {
    assert(_settings["phantom"].isBool());
    phantom_ = _settings["phantom"].asBool();
    if (phantom_) {
      assert(_settings.isMember("value_coeff") &&
             _settings["value_coeff"].isDouble());
      assert(_settings.isMember("length_coeff") &&
             _settings["length_coeff"].isDouble());
      valueCoeff_ = _settings["value_coeff"].asDouble();
      lengthCoeff_ = _settings["length_coeff"].asDouble();
      windows_.resize(totalVcs, 0);
    }
  }
}

BufferOccupancy::~BufferOccupancy() {}

void BufferOccupancy::initCredits(u32 _vcIdx, u32 _credits) {
  u32 port, vc;
  device_->vcIndexInv(_vcIdx, &port, &vc);

  assert(port < numPorts_);
  assert(vc < numVcs_);
  assert(_credits > 0);

  creditMaximums_.at(_vcIdx) += _credits;
  creditCounts_.at(_vcIdx) += _credits;
  dbgprintf("max & count on %u is now %u", _vcIdx, creditCounts_.at(_vcIdx));
}

void BufferOccupancy::incrementCredit(u32 _vcIdx) {
  createEvent(_vcIdx, INCR);
}

void BufferOccupancy::decrementCredit(u32 _vcIdx) {
  createEvent(_vcIdx, DECR);
}

void BufferOccupancy::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() > 0);
  u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
  switch (_type) {
    case INCR:
      performIncrementCredit(vcIdx);
      break;
    case DECR:
      performDecrementCredit(vcIdx);
      break;
    case PHANTOM:
      performDecrementWindow(vcIdx);
      break;
    default:
      assert(false);
  }
}

f64 BufferOccupancy::computeStatus(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  switch (mode_) {
     case BufferOccupancy::Mode::kVcNorm: {
      return vcStatusNorm(_outputPort, _outputVc);
      break;
    }
    case BufferOccupancy::Mode::kVcAbs: {
      return vcStatusAbs(_outputPort, _outputVc);
      break;
    }
    case BufferOccupancy::Mode::kPortNorm: {
      return portAverageStatusNorm(_outputPort);
      break;
    }
    case BufferOccupancy::Mode::kPortAbs: {
      return portAverageStatusAbs(_outputPort);
      break;
    }
    default:
      assert(false);
      break;
  }
}

BufferOccupancy::Mode BufferOccupancy::parseMode(const std::string& _mode) {
  if (_mode == "normalized_vc") {
    return BufferOccupancy::Mode::kVcNorm;
  } else if (_mode == "absolute_vc") {
    return  BufferOccupancy::Mode::kVcAbs;
  } else if (_mode == "normalized_port") {
    return  BufferOccupancy::Mode::kPortNorm;
  } else if (_mode == "absolute_port") {
    return  BufferOccupancy::Mode::kPortAbs;
  } else {
    assert(false);
  }
}

void BufferOccupancy::createEvent(u32 _vcIdx, s32 _type) {
  assert(gSim->epsilon() > 0);
  u64 time = latency_ == 1 ? gSim->time() :
      gSim->futureCycle(Simulator::Clock::CORE, latency_ - 1);
  addEvent(time, gSim->epsilon() + 1, reinterpret_cast<void*>(_vcIdx), _type);
}

void BufferOccupancy::performIncrementCredit(u32 _vcIdx) {
  // dbgprintf("incr %u from %u", _vcIdx, creditCounts_.at(_vcIdx));
  assert(creditCounts_.at(_vcIdx) < creditMaximums_.at(_vcIdx));
  creditCounts_.at(_vcIdx)++;
  flitsOutstanding_.at(_vcIdx)--;

  dbgprintf("(Inc) VcIdx %u credits: %u flits: %u [%u]", _vcIdx,
            creditCounts_.at(_vcIdx), flitsOutstanding_.at(_vcIdx),
            creditMaximums_.at(_vcIdx));
}

void BufferOccupancy::performDecrementCredit(u32 _vcIdx) {
  // dbgprintf("decr %u from %u", _vcIdx, creditCounts_.at(_vcIdx));
  assert(creditCounts_.at(_vcIdx) > 0);
  creditCounts_.at(_vcIdx)--;
  flitsOutstanding_.at(_vcIdx)++;

  dbgprintf("(Dec) VcIdx %u credits: %u flits: %u [%u]", _vcIdx,
            creditCounts_.at(_vcIdx), flitsOutstanding_.at(_vcIdx),
            creditMaximums_.at(_vcIdx));

  if (phantom_) {
    windows_.at(_vcIdx)++;
    u32 port, vc;
    device_->vcIndexInv(_vcIdx, &port, &vc);
    Channel* ch = device_->getOutputChannel(port);
    u32 windowLength = (u32)(ch->latency() * lengthCoeff_);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, windowLength);
    addEvent(time, gSim->epsilon(), reinterpret_cast<void*>(_vcIdx), PHANTOM);
  }
}

void BufferOccupancy::performDecrementWindow(u32 _vcIdx) {
  assert(phantom_);
  dbgprintf("-window %u from %u", _vcIdx, windows_.at(_vcIdx));
  assert(windows_.at(_vcIdx) > 0);
  windows_.at(_vcIdx)--;
}

f64 BufferOccupancy::vcStatusNorm(u32 _outputPort, u32 _outputVc) const {
  // return this VC's status (normalized)
  u32 vcIdx = device_->vcIndex(_outputPort, _outputVc);
  f64 status;
  if (!phantom_) {
    status = ((f64)creditMaximums_.at(vcIdx) - (f64)creditCounts_.at(vcIdx)) /
        (f64)creditMaximums_.at(vcIdx);
  } else {
    status = (((f64)creditMaximums_.at(vcIdx) - (f64)creditCounts_.at(vcIdx) -
               (f64)windows_.at(vcIdx) * valueCoeff_) /
              (f64)creditMaximums_.at(vcIdx));
  }

  return std::min(1.0, std::max(0.0, status));
}

f64 BufferOccupancy::vcStatusAbs(u32 _outputPort, u32 _outputVc) const {
  // return this VC's status (absolute)
  u32 vcIdx = device_->vcIndex(_outputPort, _outputVc);
  f64 statusAbs;
  if (!phantom_) {
    statusAbs = (f64)flitsOutstanding_.at(vcIdx);
  } else {
    statusAbs = (f64)flitsOutstanding_.at(vcIdx) - ((f64)windows_.at(vcIdx)
                                               * valueCoeff_);
  }
  return std::max(0.0, statusAbs);
}

f64 BufferOccupancy::portAverageStatusNorm(u32 _outputPort) const {
  // return the average status of all VCs in this port (normalized)
  u32 curSum = 0;
  u32 maxSum = 0;
  for (u32 vc = 0; vc < numVcs_; vc++) {
    u32 vcIdx = device_->vcIndex(_outputPort, vc);
    if (!phantom_) {
      curSum += creditMaximums_.at(vcIdx) - creditCounts_.at(vcIdx);
    } else {
      curSum += ((f64)creditMaximums_.at(vcIdx) - (f64)creditCounts_.at(vcIdx) -
                 (f64)windows_.at(vcIdx) * valueCoeff_);
    }
    maxSum += (f64)creditMaximums_.at(vcIdx);
  }
  return std::min(1.0, std::max(0.0, (f64)curSum / (f64)maxSum));
}

f64 BufferOccupancy::portAverageStatusAbs(u32 _outputPort) const {
  // return the average status of all VCs in this port (absolute)
  u32 curSum = 0;
  for (u32 vc = 0; vc < numVcs_; vc++) {
    u32 vcIdx = device_->vcIndex(_outputPort, vc);
    if (!phantom_) {
      curSum += (f64)flitsOutstanding_.at(vcIdx);
    } else {
      curSum += (f64)flitsOutstanding_.at(vcIdx) - ((f64)windows_.at(vcIdx)
                                            * valueCoeff_);
    }
  }
  return std::max(0.0, (f64)curSum);
}

registerWithFactory("buffer_occupancy", CongestionStatus,
                    BufferOccupancy, CONGESTIONSTATUS_ARGS);
