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
#include "workload/stream/StreamTerminal.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include "network/Network.h"
#include "stats/MessageLog.h"
#include "traffic/MessageSizeDistributionFactory.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/stream/Application.h"
#include "workload/util.h"

namespace Stream {

StreamTerminal::StreamTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, ::Application* _app,
    Json::Value _settings)
    : Terminal(_name, _parent, _id, _address, _app) {
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
    // get the injection rate
    assert(_settings.isMember("injection_rate") &&
           _settings["injection_rate"].isDouble());
    injectionRate_ = _settings["injection_rate"].asDouble();
    assert(injectionRate_ >= 0.0 && injectionRate_ <= 1.0);

    // max packet size
    maxPacketSize_ = _settings["max_packet_size"].asUInt();

    // choose a random number of cycles in the future to start
    // make an event to start the Terminal in the future
    u32 maxMsg = messageSizeDistribution_->maxMessageSize();
    u64 cycles = cyclesToSend(injectionRate_, maxMsg);
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
  if (sending_) {
    sendNextMessage();
  }
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

void StreamTerminal::handleDeliveredMessage(Message* _message) {
  // log the message
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->logMessage(_message);
}

void StreamTerminal::handleReceivedMessage(Message* _message) {
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

void StreamTerminal::sendNextMessage() {
  u64 now = gSim->time();

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

  // compute when to send next time
  u64 cycles = cyclesToSend(injectionRate_, messageLength);
  u64 time = gSim->futureCycle(Simulator::Clock::CHANNEL, cycles);
  if (time == now) {
    sendNextMessage();
  } else {
    addEvent(time, 0, nullptr, 0);
  }
}

}  // namespace Stream
