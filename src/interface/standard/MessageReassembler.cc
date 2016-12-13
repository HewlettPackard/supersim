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
#include "interface/standard/MessageReassembler.h"

#include <cassert>
#include <cstdio>

#include "workload/util.h"

namespace Standard {

MessageReassembler::MessageReassembler(const std::string& _name,
                                       const Component* _parent)
    : Component(_name, _parent) {}

MessageReassembler::~MessageReassembler() {}

Message* MessageReassembler::receivePacket(Packet* _packet) {
  // get the message and determine unqiue id
  Message* message = _packet->message();
  u32 sourceId = message->getSourceId();
  u32 messageId = message->id();
  u32 appId1 = appId(message->getTransaction());
  u64 umid = transactionId(appId1, sourceId, messageId);
  // u64 mid = ((u64)sourceId) << 32 | ((u64)messageId);

  // if non-existent, add a new table entry
  bool umidExists = messages_.count(umid) == 1;
  if (!umidExists) {
    messages_[umid] = MessageData();
    messages_[umid].message = message;
    messages_[umid].packetsReceived.resize(message->numPackets(), false);
    messages_[umid].receivedCount = 0;
  }

  // retrieve the message data
  MessageData& messageData = messages_.at(umid);
  assert(messageData.message == message);

  // mark the packet as received
  assert(messageData.packetsReceived.at(_packet->id()) == false);
  messageData.packetsReceived.at(_packet->id()) = true;
  messageData.receivedCount++;

  // check if the full message has been received
  if (messageData.receivedCount == message->numPackets()) {
    // remove the message from the map
    s32 erased = messages_.erase(umid);
    (void)erased;  // unused
    assert(erased == 1);
    return message;
  } else {
    // return nullptr to signify more packets are needed for the message
    return nullptr;
  }
}

}  // namespace Standard
