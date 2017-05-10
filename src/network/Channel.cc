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
#include "network/Channel.h"

#include <cassert>

#include "types/Flit.h"
#include "types/FlitReceiver.h"
#include "types/Credit.h"
#include "types/CreditReceiver.h"
#include "types/Packet.h"
#include "event/Simulator.h"

#define FLIT 0xBE
#define CRDT 0xEF

Channel::Channel(const std::string& _name, const Component* _parent,
                 u32 _numVcs, Json::Value _settings)
    : Component(_name, _parent),
      latency_(_settings["latency"].asUInt()),
      numVcs_(_numVcs) {
  assert(!_settings["latency"].isNull());
  assert(latency_ > 0);
  assert(numVcs_ > 0);

  nextFlitTime_ = U64_MAX;
  nextFlit_ = nullptr;
  nextCreditTime_ = U64_MAX;
  nextCredit_ = nullptr;

  monitoring_ = false;
  monitorTime_ = U64_MAX;
  monitorCounts_.resize(_numVcs + 1);
}

Channel::~Channel() {}

u32 Channel::latency() const {
  return latency_;
}

void Channel::setSource(CreditReceiver* _source, u32 _port) {
  source_ = _source;
  sourcePort_ = _port;
}

void Channel::setSink(FlitReceiver* _sink, u32 _port) {
  sink_ = _sink;
  sinkPort_ = _port;
}

void Channel::startMonitoring() {
  assert(monitoring_ == false);
  assert(monitorTime_ == U64_MAX);
  monitoring_ = true;
  monitorTime_ = gSim->time();  // start time
  for (auto& mc : monitorCounts_) {
    mc = 0;
  }
}

void Channel::endMonitoring() {
  assert(monitoring_ == true);
  assert(monitorTime_ != U64_MAX);
  monitoring_ = false;
  monitorTime_ = gSim->time() - monitorTime_;  // delta time
}

f64 Channel::utilization(u32 _vc) const {
  assert(monitoring_ == false);
  assert(monitorTime_ != U64_MAX);
  u64 count;
  if (_vc == U32_MAX) {
    count = monitorCounts_.at(numVcs_);
  } else {
    assert(_vc < numVcs_);
    count = monitorCounts_.at(_vc);
  }
  return (f64)count / ((f64)monitorTime_ / gSim->cycleTime(
      Simulator::Clock::CHANNEL));
}

void Channel::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() == 1);
  switch (_type) {
    case FLIT:
      {
        Flit* flit = reinterpret_cast<Flit*>(_event);
        if (flit->isHead()) {
          flit->packet()->incrementHopCount();
        }
        sink_->receiveFlit(sinkPort_, flit);
      }
      break;
    case CRDT:
      {
        Credit* credit = reinterpret_cast<Credit*>(_event);
        source_->receiveCredit(sourcePort_, credit);
      }
      break;
    default:
      assert(false);
  }
}

Flit* Channel::getNextFlit() const {
  // determine the next time slot to send a flit
  u64 nextSlot = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);

  // return nullptr if the next flit hasn't been set
  if (nextFlitTime_ != nextSlot) {
    return nullptr;
  } else {
    // if it was set, return it
    return nextFlit_;
  }
}

u64 Channel::setNextFlit(Flit* _flit) {
  // determine the next time slot to send a flit
  u64 nextSlot = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
  assert(nextSlot != nextFlitTime_);

  // set the time and value
  nextFlitTime_ = nextSlot;
  nextFlit_ = _flit;

  // add the event of when the flit will arrive on the other end
  u64 nextTime = gSim->futureCycle(Simulator::Clock::CHANNEL, latency_);
  addEvent(nextTime, 1, _flit, FLIT);

  // increment the count when monitoring
  assert(_flit->getVc() < numVcs_);
  if (monitoring_) {
    monitorCounts_.at(_flit->getVc())++;
    monitorCounts_.at(numVcs_)++;
  }

  // return the injection time
  return nextFlitTime_;
}

Credit* Channel::getNextCredit() const {
  // determine the next time slot to send a credit
  u64 nextSlot = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);

  // return nullptr if the next credit hasn't been set
  if (nextCreditTime_ != nextSlot) {
    return nullptr;
  } else {
    // if it was set, return it
    return nextCredit_;
  }
}

u64 Channel::setNextCredit(Credit* _credit) {
  // determine the next time slot to send a credit
  u64 nextSlot = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
  assert(nextSlot != nextCreditTime_);

  // set the time and value
  nextCreditTime_ = nextSlot;
  nextCredit_ = _credit;

  // add the event of when the credit will arrive on the other end
  u64 nextTime = gSim->futureCycle(Simulator::Clock::CHANNEL, latency_);
  addEvent(nextTime, 1, _credit, CRDT);

  // return the injection time
  return nextCreditTime_;
}
