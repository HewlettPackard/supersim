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
#ifndef INTERFACE_INTERFACE_H_
#define INTERFACE_INTERFACE_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "types/FlitReceiver.h"
#include "types/FlitSender.h"
#include "types/CreditReceiver.h"
#include "types/CreditSender.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"
#include "network/Channel.h"

class PacketReassembler;
class MessageReassembler;

class Interface : public Component, public FlitSender, public FlitReceiver,
                  public CreditSender, public CreditReceiver,
                  public MessageReceiver {
 public:
  Interface(const std::string& _name, const Component* _parent, u32 _id,
            Json::Value _settings);
  virtual ~Interface();
  u32 getId() const;
  u32 numVcs() const;
  void setMessageReceiver(MessageReceiver* _receiver);
  MessageReceiver* getMessageReceiver() const;
  virtual void setInputChannel(Channel* _channel) = 0;
  virtual void setOutputChannel(Channel* _channel) = 0;

 protected:
  const u32 id_;
  const u32 numVcs_;

 private:
  MessageReceiver* messageReceiver_;
  std::vector<PacketReassembler*> packetReassemblers_;
  MessageReassembler* messageReassembler_;
};

#endif  // INTERFACE_INTERFACE_H_
