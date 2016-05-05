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
#include "router/common/CongestionStatus.h"

#include <cassert>

const s32 INCR = 0x50;
const s32 DECR = 0xAF;

CongestionStatus::CongestionStatus(
    const std::string& _name, const Component* _parent, u32 _totalVcs,
    Json::Value _settings)
    : Component(_name, _parent), totalVcs_(_totalVcs),
      latency_(_settings["latency"].asUInt()) {
  assert(latency_ > 0);
  maximums_.resize(totalVcs_);
  counts_.resize(totalVcs_);
}

CongestionStatus::~CongestionStatus() {}

void CongestionStatus::initCredits(u32 _vcIdx, u32 _credits) {
  assert(_credits > 0);
  maximums_.at(_vcIdx) = _credits;
  counts_.at(_vcIdx) = _credits;
}

void CongestionStatus::increment(u32 _vcIdx) {
  createEvent(_vcIdx, INCR);
}

void CongestionStatus::decrement(u32 _vcIdx) {
  createEvent(_vcIdx, DECR);
}

f64 CongestionStatus::status(u32 _vcIdx) const {
  assert(gSim->epsilon() == 0);
  return (f64)counts_.at(_vcIdx) / (f64)maximums_.at(_vcIdx);
}

void CongestionStatus::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() > 0);
  u32 vcIdx = static_cast<u32>(reinterpret_cast<u64>(_event));
  switch (_type) {
    case INCR:
      dbgprintf("incr %u from %u", vcIdx, counts_.at(vcIdx));
      assert(counts_.at(vcIdx) < maximums_.at(vcIdx));
      counts_.at(vcIdx)++;
      break;
    case DECR:
      dbgprintf("decr %u from %u", vcIdx, counts_.at(vcIdx));
      assert(counts_.at(vcIdx) > 0);
      counts_.at(vcIdx)--;
      break;
    default:
      assert(false);
  }
}

void CongestionStatus::createEvent(u32 _vcIdx, s32 _type) {
  assert(gSim->epsilon() > 0);
  u64 time = latency_ == 1 ? gSim->time() : gSim->futureCycle(latency_ - 1);
  addEvent(time, gSim->epsilon() + 1, reinterpret_cast<void*>(_vcIdx), _type);
}
