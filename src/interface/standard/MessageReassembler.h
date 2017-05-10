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
#ifndef INTERFACE_STANDARD_MESSAGEREASSEMBLER_H_
#define INTERFACE_STANDARD_MESSAGEREASSEMBLER_H_

#include <string>
#include <vector>
#include <unordered_map>

#include "event/Component.h"
#include "types/Packet.h"
#include "types/Message.h"

namespace Standard {

class MessageReassembler : public Component {
 public:
  MessageReassembler(const std::string& _name, const Component* _parent);
  ~MessageReassembler();
  Message* receivePacket(Packet* _packet);

 private:
  struct MessageData {
    Message* message;
    std::vector<bool> packetsReceived;
    u32 receivedCount;
  };
  std::unordered_map<u64, MessageData> messages_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_MESSAGEREASSEMBLER_H_
