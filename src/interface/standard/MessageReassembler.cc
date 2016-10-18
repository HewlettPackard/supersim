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
  u64 mid = ((u64)sourceId) << 32 | ((u64)messageId);

  // if non-existent, add a new table entry
  bool midExists = messages_.count(mid) == 1;
  if (!midExists) {
    messages_[mid] = MessageData();
    messages_[mid].message = message;
    messages_[mid].packetsReceived.resize(message->numPackets(), false);
    messages_[mid].receivedCount = 0;
  }

  // retrieve the message data
  MessageData& messageData = messages_.at(mid);
  assert(messageData.message == message);

  // mark the packet as received
  assert(messageData.packetsReceived.at(_packet->id()) == false);
  messageData.packetsReceived.at(_packet->id()) = true;
  messageData.receivedCount++;

  // check if the full message has been received
  if (messageData.receivedCount == message->numPackets()) {
    // remove the message from the map
    s32 erased = messages_.erase(mid);
    (void)erased;  // unused
    assert(erased == 1);
    return message;
  } else {
    // return nullptr to signify more packets are needed for the message
    return nullptr;
  }
}

}  // namespace Standard
