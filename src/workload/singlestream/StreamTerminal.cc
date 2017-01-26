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
#include "workload/singlestream/StreamTerminal.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include "workload/singlestream/Application.h"
#include "network/Network.h"
#include "stats/MessageLog.h"
#include "traffic/MessageSizeDistributionFactory.h"
#include "types/Flit.h"
#include "types/Packet.h"

namespace SingleStream {

StreamTerminal::StreamTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, ::Application* _app,
    Json::Value _settings)
    : Terminal(_name, _parent, _id, _address, _app), lastSendTime_(0) {
  // create a message size distribution
  messageSizeDistribution_ = MessageSizeDistributionFactory::
      createMessageSizeDistribution("MessageSizeDistribution", this,
                                    _settings["message_size_distribution"]);

  // traffic class of injection
  assert(_settings.isMember("traffic_class"));
  trafficClass_ = _settings["traffic_class"].asUInt();

  // message quantity limition
  numMessages_ = _settings["num_messages"].asUInt();

  // apply the routines only if this terminal is targeted
  Application* app = reinterpret_cast<Application*>(application());
  if (_id == app->getSource()) {
    // this application requires an injection rate for the source
    assert(app->maxInjectionRate(_id) > 0.0);

    // max packet size
    maxPacketSize_  = _settings["max_packet_size"].asUInt();

    // choose a random number of cycles in the future to start
    // make an event to start the Terminal in the future
    u32 maxMsg = messageSizeDistribution_->maxMessageSize();
    u64 cycles = application()->cyclesToSend(_id, maxMsg);
    cycles = gSim->rnd.nextU64(1, 1 + cycles * 3);
    u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, 1) +
        ((cycles - 1) * gSim->cycleTime(Simulator::Clock::CHANNEL));
    dbgprintf("start time is %lu", time);
    addEvent(time, 0, nullptr, 0);
  }
  recvdMessages_ = 0;
  destReady_ = false;
  destComplete_ = false;

  counting_ = false;
  sending_ = true;
}

StreamTerminal::~StreamTerminal() {
  delete messageSizeDistribution_;
}

void StreamTerminal::processEvent(void* _event, s32 _type) {
  sendNextMessage();
}

void StreamTerminal::receiveMessage(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::receiveMessage(_message);

  dbgprintf("received message %u", _message->id());

  // end the transaction
  endTransaction(_message->getTransaction());
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->endTransaction(_message->getTransaction());

  // signal the app if this is the first receive
  if (!destReady_) {
    destReady_ = true;
    app->destinationReady(id_);
  }

  // count receive messages
  if (counting_) {
    recvdMessages_++;
  }

  // check when done
  if (counting_ && recvdMessages_ >= numMessages_ && !destComplete_) {
    Application* app = reinterpret_cast<Application*>(application());
    app->destinationComplete(id_);
    destComplete_ = true;
  }

  delete _message;  // don't need this anymore
}

void StreamTerminal::messageEnteredInterface(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageEnteredInterface(_message);

  // determine if more messages should be created and sent
  if (sending_) {
    u64 now = gSim->time();
    assert(lastSendTime_ <= now);
    if (now == lastSendTime_) {
      addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, 1), 0, nullptr, 0);
    } else {
      sendNextMessage();
    }
  }
}

void StreamTerminal::messageExitedNetwork(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageExitedNetwork(_message);

  // log the message
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->logMessage(_message);
}

f64 StreamTerminal::percentComplete() const {
  if (!counting_) {
    return 0.0;
  } else if (numMessages_ == 0) {
    return 1.0;
  } else {
    u32 count = std::min(numMessages_, recvdMessages_);
    return (f64)count / (f64)numMessages_;
  }
}

void StreamTerminal::start() {
  counting_ = true;
}

void StreamTerminal::stop() {
  sending_ = false;
}

void StreamTerminal::sendNextMessage() {
  u64 now = gSim->time();
  lastSendTime_ = now;

  // pick a destination
  Application* app = reinterpret_cast<Application*>(application());
  u32 destination = app->getDestination();

  // pick a random message length
  u32 messageLength = messageSizeDistribution_->nextMessageSize();
  u32 numPackets = messageLength / maxPacketSize_;
  if ((messageLength % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setTrafficClass(trafficClass_);
  u64 trans = createTransaction();
  message->setTransaction(trans);
  app->workload()->messageLog()->startTransaction(trans);

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
}

}  // namespace SingleStream
