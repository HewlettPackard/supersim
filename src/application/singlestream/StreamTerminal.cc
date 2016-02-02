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
#include "application/singlestream/StreamTerminal.h"

#include <cassert>
#include <cmath>

#include "application/singlestream/Application.h"
#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"

namespace SingleStream {

StreamTerminal::StreamTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    std::vector<u32> _address, ::Application* _app, Json::Value _settings)
    : Terminal(_name, _parent, _id, _address, _app), lastSendTime_(0) {
  // message quantity limition
  numMessages_ = _settings["num_messages"].asUInt();

  // apply the routines only if this terminal is targeted
  Application* app = reinterpret_cast<Application*>(getApplication());
  if (_id == app->getSource()) {
    remainingMessages_ = numMessages_;

    // message and packet sizes
    minMessageSize_ = _settings["min_message_size"].asUInt();
    maxMessageSize_ = _settings["max_message_size"].asUInt();
    maxPacketSize_  = _settings["max_packet_size"].asUInt();

    // choose a random number of cycles in the future to start
    // make an event to start the Terminal in the future
    u64 cycles = getApplication()->cyclesToSend(maxMessageSize_);
    cycles = gSim->rnd.nextU64(1, cycles);
    u64 time = gSim->futureCycle(1) + ((cycles - 1) * gSim->cycleTime());
    dbgprintf("start time is %lu", time);
    addEvent(time, 0, nullptr, 0);
    remainingMessages_--;  // decrement for first message
  } else {
    remainingMessages_ = 0;
  }
  recvdMessages_ = 0;
}

StreamTerminal::~StreamTerminal() {}

void StreamTerminal::processEvent(void* _event, s32 _type) {
  sendNextMessage();
}

void StreamTerminal::handleMessage(Message* _message) {
  dbgprintf("received message %u", _message->getId());

  // end the transaction
  endTransaction(_message->getTransaction());

  // determine when complete
  recvdMessages_++;

  // on first received message, start monitoring
  if (recvdMessages_ == 1) {
    Application* app = reinterpret_cast<Application*>(getApplication());
    app->receivedFirst(getId());
  }

  delete _message;  // don't need this anymore
}

void StreamTerminal::messageEnteredInterface(Message* _message) {
  // determine if more messages should be created and sent
  u64 now = gSim->time();
  assert(lastSendTime_ <= now);
  if (remainingMessages_ > 0) {
    remainingMessages_--;
    if (now == lastSendTime_) {
      addEvent(gSim->futureCycle(1), 0, nullptr, 0);
    } else {
      sendNextMessage();
    }
  } else {
    Application* app = reinterpret_cast<Application*>(getApplication());
    app->sentLast(getId());
  }
}

void StreamTerminal::messageExitedNetwork(Message* _message) {
  // any override of this function must call the base class's function
  ::Terminal::messageExitedNetwork(_message);

  // log the message
  getApplication()->getMessageLog()->logMessage(_message);
}

f64 StreamTerminal::percentComplete() const {
  return (f64)recvdMessages_ / (f64)numMessages_;
}

void StreamTerminal::sendNextMessage() {
  u64 now = gSim->time();
  lastSendTime_ = now;

  // pick a destination
  Application* app = reinterpret_cast<Application*>(getApplication());
  u32 destination = app->getDestination();

  // pick a random message length
  u32 messageLength = gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
  u32 numPackets = messageLength / maxPacketSize_;
  if ((messageLength % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setTransaction(createTransaction());

  // create the packets
  u32 flitsLeft = messageLength;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ?
                       maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // pick a random starting VC
    u32 numVcs = gSim->getNetwork()->numVcs();
    u32 vc = gSim->rnd.nextU64(0, numVcs - 1);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      flit->setVc(vc);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the message
  sendMessage(message, destination);
}

}  // namespace SingleStream
