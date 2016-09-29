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
#include "application/stresstest/BlastTerminal.h"

#include <mut/mut.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "application/stresstest/Application.h"
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
      "TrafficPattern", this, getApplication()->numTerminals(), getId(),
      _settings["traffic_pattern"]);

  // message quantity limition
  numMessages_ = _settings["num_messages"].asUInt();
  remainingMessages_ = numMessages_;

  // message and packet sizes
  minMessageSize_ = _settings["min_message_size"].asUInt();
  maxMessageSize_ = _settings["max_message_size"].asUInt();
  maxPacketSize_  = _settings["max_packet_size"].asUInt();

  // warmup/saturation detector
  warmupEnable_ = true;
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
  f64 mir = getApplication()->maxInjectionRate(_id);
  u64 cycles = getApplication()->cyclesToSend(maxMessageSize_, mir);
  cycles = gSim->rnd.nextU64(1, 1 + cycles * 3);
  u64 time = gSim->futureCycle(1) + ((cycles - 1) * gSim->cycleTime());
  dbgprintf("start time is %lu", time);
  addEvent(time, 0, nullptr, 0);

  // initialize the FSM
  enableLogging_ = false;
  enableSending_ = true;
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

  if (messagesToLog_.count(_message->getId()) == 1) {
    loggableEnteredCount_++;
    dbgprintf("loggable entered network (%u of %u)",
              loggableEnteredCount_, numMessages_);
  }

  // determine if more messages should be created and sent
  if (enableSending_) {
    u64 now = gSim->time();
    assert(lastSendTime_ <= now);
    if (now == lastSendTime_) {
      addEvent(gSim->futureCycle(1), 0, nullptr, 0);
    } else {
      sendNextMessage();
    }
  }
}

void BlastTerminal::messageExitedNetwork(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageExitedNetwork(_message);

  Application* app = reinterpret_cast<Application*>(getApplication());

  // process for each warmup window
  if (warmupEnable_) {
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
        enrouteSampleTimes_.push_back(gSim->cycle());
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
      if (warmed) {
        // the network is warmed up
        dbgprintf("warmed");
        app->terminalWarmed(getId());
        warmupEnable_ = false;
        enrouteSampleTimes_.clear();
        enrouteSampleValues_.clear();
      } else if (saturated) {
        // the network is saturated
        dbgprintf("saturated");
        app->terminalSaturated(getId());
        warmupEnable_ = false;
        enrouteSampleTimes_.clear();
        enrouteSampleValues_.clear();
      }
    }
  }

  // log message if tagged
  u32 mId = _message->getId();
  if (messagesToLog_.count(mId) == 1) {
    assert(enableLogging_);
    assert(messagesToLog_.erase(mId) == 1);

    // log the message/transaction
    app->getMessageLog()->logMessage(_message);
    app->getMessageLog()->endTransaction(_message->getTransaction());
    loggableExitedCount_++;

    // detect when done
    if (loggableExitedCount_ == numMessages_) {
      assert(messagesToLog_.size() == 0);
      app->terminalComplete(getId());
    }
  }
}

f64 BlastTerminal::percentComplete() const {
  if (!enableLogging_) {
    return 0.0;
  } else {
    return (f64)loggableExitedCount_ / (f64)numMessages_;
  }
}

void BlastTerminal::startLogging() {
  enableLogging_ = true;
  warmupEnable_ = false;
}

void BlastTerminal::stopSending() {
  enableSending_ = false;
  warmupEnable_ = false;
  enrouteSampleTimes_.clear();
  enrouteSampleValues_.clear();
}

void BlastTerminal::sendNextMessage() {
  u64 now = gSim->time();
  lastSendTime_ = now;

  Application* app = reinterpret_cast<Application*>(getApplication());

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
  if ((enableLogging_) && (remainingMessages_ > 0)) {
    remainingMessages_--;
    messagesToLog_.insert(msgId);
    app->getMessageLog()->startTransaction(trans);
  }
}

}  // namespace StressTest
