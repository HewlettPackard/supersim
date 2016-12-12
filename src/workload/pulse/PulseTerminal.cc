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
#include "workload/pulse/PulseTerminal.h"

#include <mut/mut.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "workload/pulse/Application.h"
#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "traffic/TrafficPatternFactory.h"

namespace Pulse {

PulseTerminal::PulseTerminal(const std::string& _name, const Component* _parent,
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
  assert(_settings.isMember("fake_responses"));
  fakeResponses_ = _settings["fake_responses"].asBool();
  assert(minMessageSize_ > 0);
  assert(maxMessageSize_ >= minMessageSize_);
  assert(maxPacketSize_ > 0);

  // start time delay
  assert(_settings.isMember("delay"));
  delay_ = _settings["delay"].asUInt();

  // initialize the counters
  loggableEnteredCount_ = 0;
  loggableExitedCount_ = 0;
}

PulseTerminal::~PulseTerminal() {
  delete trafficPattern_;
}

void PulseTerminal::processEvent(void* _event, s32 _type) {
  sendNextMessage();
}

void PulseTerminal::receiveMessage(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::receiveMessage(_message);

  // end the transaction
  endTransaction(_message->getTransaction());

  delete _message;  // don't need this anymore
}

void PulseTerminal::messageEnteredInterface(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageEnteredInterface(_message);

  // count
  loggableEnteredCount_++;
  dbgprintf("loggable entered network (%u of %u)",
            loggableEnteredCount_, numMessages_);

  // determine if more messages should be created and sent
  if (loggableEnteredCount_ < numMessages_) {
    u64 now = gSim->time();
    assert(lastSendTime_ <= now);
    if (now == lastSendTime_) {
      addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, 1), 0, nullptr, 0);
    } else {
      sendNextMessage();
    }
  }
}

void PulseTerminal::messageExitedNetwork(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageExitedNetwork(_message);

  Application* app = reinterpret_cast<Application*>(application());

  // log the message/transaction
  app->workload()->messageLog()->logMessage(_message);
  app->workload()->messageLog()->endTransaction(_message->getTransaction());
  loggableExitedCount_++;

  // determine when complete
  if (loggableExitedCount_ == numMessages_) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  }
}

f64 PulseTerminal::percentComplete() const {
  if (numMessages_ == 0) {
    return 1.0;
  } else {
    u32 count = std::min(loggableExitedCount_, numMessages_);
    return (f64)count / (f64)numMessages_;
  }
}

void PulseTerminal::start() {
  Application* app = reinterpret_cast<Application*>(application());

  if (numMessages_ == 0) {
    dbgprintf("complete");
    app->terminalComplete(id_);
  } else {
    // choose a random number of cycles in the future to start
    // make an event to start the PulseTerminal in the future
    if (application()->maxInjectionRate(id_) > 0.0) {
      u64 cycles = application()->cyclesToSend(id_, maxMessageSize_);
      cycles = gSim->rnd.nextU64(delay_, delay_ + cycles * 3);
      u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
          ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
      dbgprintf("start time is %lu", time);
      addEvent(time, 0, nullptr, 0);
    } else {
      dbgprintf("not running");
    }
  }
}

void PulseTerminal::sendNextMessage() {
  u64 now = gSim->time();
  lastSendTime_ = now;

  Application* app = reinterpret_cast<Application*>(application());

  // pick a destination
  u32 destination = trafficPattern_->nextDestination();
  assert(destination < app->numTerminals());

  // pick a message length
  u32 messageLength;
  if (fakeResponses_ && gSim->rnd.nextBool()) {
    messageLength = 1;
  } else {
    messageLength = gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
  }

  // determine the number of packets
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
  sendMessage(message, destination);

  // start the transaction
  app->workload()->messageLog()->startTransaction(trans);
}

}  // namespace Pulse
