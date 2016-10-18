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
#include "workload/stresstest/BlastTerminal.h"

#include <mut/mut.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "workload/stresstest/Application.h"
#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "traffic/TrafficPatternFactory.h"

namespace StressTest {

BlastTerminal::BlastTerminal(const std::string& _name, const Component* _parent,
                             u32 _id, const std::vector<u32>& _address,
                             ::Application* _app, Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app),
      lastSendTime_(0) {
  // create a traffic pattern
  trafficPattern_ = TrafficPatternFactory::createTrafficPattern(
      "TrafficPattern", this, application()->numTerminals(), id_,
      _settings["traffic_pattern"]);

  // traffic class of injection
  assert(_settings.isMember("traffic_class"));
  trafficClass_ = _settings["traffic_class"].asUInt();

  // message quantity limitation
  assert(_settings.isMember("num_messages"));
  numMessages_ = _settings["num_messages"].asUInt();

  // message and packet sizes
  minMessageSize_ = _settings["min_message_size"].asUInt();
  maxMessageSize_ = _settings["max_message_size"].asUInt();
  maxPacketSize_  = _settings["max_packet_size"].asUInt();
  assert(minMessageSize_ > 0);
  assert(maxMessageSize_ >= minMessageSize_);
  assert(maxPacketSize_ > 0);

  // warmup/saturation detector
  fsm_ = BlastTerminal::Fsm::WARMING;
  warmupInterval_ = _settings["warmup_interval"].asUInt();
  assert(warmupInterval_ > 0);
  warmupFlitsReceived_ = 0;
  warmupWindow_ = _settings["warmup_window"].asUInt();
  assert(warmupWindow_ >= 5);
  maxWarmupAttempts_ = _settings["warmup_attempts"].asUInt();
  assert(maxWarmupAttempts_ > 0);
  warmupAttempts_ = 0;
  enrouteSamplePos_ = 0;
  fastFailSample_ = U32_MAX;

  // choose a random number of cycles in the future to start
  // make an event to start the BlastTerminal in the future
  if (application()->maxInjectionRate(_id) > 0.0) {
    u64 cycles = application()->cyclesToSend(_id, maxMessageSize_);
    cycles = gSim->rnd.nextU64(1, 1 + cycles * 3);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
        ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
    dbgprintf("start time is %lu", time);
    addEvent(time, 0, nullptr, 0);
  } else {
    dbgprintf("not running");
  }

  // initialize the counters
  loggableEnteredCount_ = 0;
  loggableExitedCount_ = 0;
}

BlastTerminal::~BlastTerminal() {
  delete trafficPattern_;
}

void BlastTerminal::processEvent(void* _event, s32 _type) {
  sendNextMessage();
}

void BlastTerminal::receiveMessage(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::receiveMessage(_message);

  // end the transaction
  endTransaction(_message->getTransaction());

  delete _message;  // don't need this anymore
}

void BlastTerminal::messageEnteredInterface(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageEnteredInterface(_message);

  if (messagesToLog_.count(_message->id()) == 1) {
    loggableEnteredCount_++;
    dbgprintf("loggable entered network (%u of %u)",
              loggableEnteredCount_, numMessages_);
  }

  // determine if more messages should be created and sent
  if (fsm_ != BlastTerminal::Fsm::DRAINING) {
    u64 now = gSim->time();
    assert(lastSendTime_ <= now);
    if (now == lastSendTime_) {
      addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, 1), 0, nullptr, 0);
    } else {
      sendNextMessage();
    }
  }
}

void BlastTerminal::messageExitedNetwork(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageExitedNetwork(_message);

  Application* app = reinterpret_cast<Application*>(application());

  // process for each warmup window
  if (fsm_ == BlastTerminal::Fsm::WARMING) {
    // count flits received
    warmupFlitsReceived_ += _message->numFlits();
    if (warmupFlitsReceived_ >= warmupInterval_) {
      warmupFlitsReceived_ %= warmupInterval_;

      u32 msgs;
      u32 pkts;
      u32 flits;
      enrouteCount(&msgs, &pkts, &flits);
      dbgprintf("enroute: msgs=%u pkts=%u flits=%u", msgs, pkts, flits);

      // push this sample into the cyclic buffers
      if (enrouteSampleTimes_.size() < warmupWindow_) {
        enrouteSampleTimes_.push_back(gSim->cycle(Simulator::Clock::CHANNEL));
        enrouteSampleValues_.push_back(flits);
      } else {
        enrouteSampleTimes_.at(enrouteSamplePos_) = gSim->time();
        enrouteSampleValues_.at(enrouteSamplePos_) = flits;
        enrouteSamplePos_ = (enrouteSamplePos_ + 1) % warmupWindow_;
      }

      bool warmed = false;
      bool saturated = false;

      // run the fast fail logic for early saturation detection
      if (enrouteSampleTimes_.size() == warmupWindow_) {
        if (fastFailSample_ == U32_MAX) {
          fastFailSample_ = *std::max_element(enrouteSampleValues_.begin(),
                                              enrouteSampleValues_.end());
          dbgprintf("fast fail sample = %u", fastFailSample_);
        } else if (flits > (fastFailSample_ * 3)) {
          dbgprintf("fast fail detected");
          saturated = true;
        }
      }

      // after enough samples were taken, try to figure out network status using
      //  a sliding window linear regression
      if (enrouteSampleTimes_.size() == warmupWindow_) {
        warmupAttempts_++;
        dbgprintf("warmup attempt %u of %u",
                  warmupAttempts_, maxWarmupAttempts_);
        f64 growthRate = mut::slope<u64>(enrouteSampleTimes_,
                                         enrouteSampleValues_);
        dbgprintf("growthRate: %e", growthRate);
        if (growthRate <= 0.0) {
          warmed = true;
        } else if (warmupAttempts_ == maxWarmupAttempts_) {
          saturated = true;
        }
      }

      // react to warmed or saturated
      if (warmed || saturated) {
        warm(saturated);
      }
    }
  }

  // log message if tagged
  u32 mId = _message->id();
  if (messagesToLog_.count(mId) == 1) {
    assert(messagesToLog_.erase(mId) == 1);

    // log the message/transaction
    app->workload()->messageLog()->logMessage(_message);
    app->workload()->messageLog()->endTransaction(_message->getTransaction());
    loggableExitedCount_++;

    // detect when logging complete
    if (loggableExitedCount_ == numMessages_) {
      complete();
    }

    // detect when logging is empty
    if ((fsm_ == BlastTerminal::Fsm::LOG_BLABBING) &&
        (messagesToLog_.size() == 0)) {
      done();
    }
  }
}

f64 BlastTerminal::percentComplete() const {
  if (fsm_ >= BlastTerminal::Fsm::LOGGING) {
    if (numMessages_ == 0) {
      return 1.0;
    } else {
      u32 count = std::min(loggableExitedCount_, numMessages_);
      return (f64)count / (f64)numMessages_;
    }
  } else {
    return 0.0;
  }
}

void BlastTerminal::stopWarming() {
  fsm_ = BlastTerminal::Fsm::WARM_BLABBING;
}

void BlastTerminal::startLogging() {
  // clear the samples in case it hasn't already happened
  enrouteSampleTimes_.clear();
  enrouteSampleValues_.clear();

  fsm_ = BlastTerminal::Fsm::LOGGING;
  if (numMessages_ == 0) {
    complete();
  }
}

void BlastTerminal::stopLogging() {
  fsm_ = BlastTerminal::Fsm::LOG_BLABBING;
  if (numMessages_ == 0) {
    done();
  }
}

void BlastTerminal::stopSending() {
  fsm_ = BlastTerminal::Fsm::DRAINING;
}

void BlastTerminal::warm(bool _saturated) {
  fsm_ = BlastTerminal::Fsm::WARM_BLABBING;
  Application* app = reinterpret_cast<Application*>(application());
  if (_saturated) {
    dbgprintf("saturated");
    app->terminalSaturated(id_);
  } else {
    dbgprintf("warmed");
    app->terminalWarmed(id_);
  }
  enrouteSampleTimes_.clear();
  enrouteSampleValues_.clear();
}

void BlastTerminal::complete() {
  dbgprintf("complete");
  Application* app = reinterpret_cast<Application*>(application());
  app->terminalComplete(id_);
}

void BlastTerminal::done() {
  dbgprintf("done");
  Application* app = reinterpret_cast<Application*>(application());
  app->terminalDone(id_);
}

void BlastTerminal::sendNextMessage() {
  u64 now = gSim->time();
  lastSendTime_ = now;

  Application* app = reinterpret_cast<Application*>(application());

  // pick a destination
  u32 destination = trafficPattern_->nextDestination();
  assert(destination < app->numTerminals());

  // pick a random message length
  u32 messageLength = gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
  u32 numPackets = messageLength / maxPacketSize_;
  if ((messageLength % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setTrafficClass(trafficClass_);
  u64 trans = createTransaction();
  message->setTransaction(trans);

  // create the packets
  u32 flitsLeft = messageLength;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ?
        maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the message
  u32 msgId = sendMessage(message, destination);

  // determine if this message/transaction should be logged
  if (fsm_ == BlastTerminal::Fsm::LOGGING) {
    messagesToLog_.insert(msgId);
    app->workload()->messageLog()->startTransaction(trans);
  }
}

}  // namespace StressTest
