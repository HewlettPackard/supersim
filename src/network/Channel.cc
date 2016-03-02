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
#include "network/Channel.h"

#include <cassert>

#include "types/Flit.h"
#include "types/FlitReceiver.h"
#include "types/Credit.h"
#include "types/CreditReceiver.h"
#include "types/Packet.h"
#include "event/Simulator.h"

#define FLIT 0xBE
#define CTRL 0xEF

Channel::Channel(const std::string& _name, const Component* _parent,
                 Json::Value _settings)
    : Component(_name, _parent) {
  latency_ = _settings["latency"].asUInt();
  assert(latency_ > 0);

  nextFlitTime_ = U64_MAX;
  nextFlit_ = nullptr;
  nextCreditTime_ = U64_MAX;
  nextCredit_ = nullptr;

  monitoring_ = false;
  monitorTime_ = U64_MAX;
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
  monitorCount_ = 0;
}

void Channel::endMonitoring() {
  assert(monitoring_ == true);
  assert(monitorTime_ != U64_MAX);
  monitoring_ = false;
  monitorTime_ = gSim->time() - monitorTime_;  // delta time
}

f64 Channel::utilization() const {
  assert(monitoring_ == false);
  assert(monitorTime_ != U64_MAX);
  return (f64)monitorCount_ / ((f64)monitorTime_ / gSim->cycleTime());
}

void Channel::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() == 1);
  switch (_type) {
    case FLIT:
      {
        Flit* flit = reinterpret_cast<Flit*>(_event);
        if (flit->isHead()) {
          flit->getPacket()->incrementHopCount();
        }
        sink_->receiveFlit(sinkPort_, flit);
      }
      break;
    case CTRL:
      {
        source_->receiveCredit(sourcePort_,
                               reinterpret_cast<Credit*>(_event));
      }
      break;
    default:
      assert(false);
  }
}

Flit* Channel::getNextFlit() const {
  // determine the next time slot to send a flit
  u64 nextSlot = gSim->futureCycle(1);

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
  u64 nextSlot = gSim->futureCycle(1);
  assert(nextSlot != nextFlitTime_);

  // set the time and value
  nextFlitTime_ = nextSlot;
  nextFlit_ = _flit;

  // add the event of when the flit will arrive on the other end
  u64 nextTime = gSim->futureCycle(latency_);
  addEvent(nextTime, 1, _flit, FLIT);

  // increment the count when monitoring
  if (monitoring_) {
    monitorCount_++;
  }

  // return the injection time
  return nextFlitTime_;
}

Credit* Channel::getNextCredit() const {
  // determine the next time slot to send a credit
  u64 nextSlot = gSim->futureCycle(1);

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
  u64 nextSlot = gSim->futureCycle(1);
  assert(nextSlot != nextCreditTime_);

  // set the time and value
  nextCreditTime_ = nextSlot;
  nextCredit_ = _credit;

  // add the event of when the credit will arrive on the other end
  u64 nextTime = gSim->futureCycle(latency_);
  addEvent(nextTime, 1, _credit, CTRL);

  // return the injection time
  return nextCreditTime_;
}
