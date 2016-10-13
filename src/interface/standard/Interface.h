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
#ifndef INTERFACE_STANDARD_INTERFACE_H_
#define INTERFACE_STANDARD_INTERFACE_H_

#include <json/json.h>
#include <string>
#include <tuple>
#include <vector>

#include "interface/Interface.h"
#include "network/Channel.h"
#include "network/InjectionAlgorithm.h"
#include "network/InjectionAlgorithmFactory.h"
#include "router/common/Crossbar.h"
#include "router/common/CrossbarScheduler.h"
#include "types/Credit.h"
#include "types/CreditReceiver.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"

namespace Standard {

class InputQueue;
class Ejector;
class PacketReassembler;
class MessageReassembler;

class Interface : public ::Interface, public InjectionAlgorithm::Client {
 public:
  Interface(const std::string& _name, const Component* _parent, u32 _numVcs,
            u32 _id, InjectionAlgorithmFactory* _injectionAlgorithmFactory,
            Json::Value _settings);
  ~Interface();
  void setInputChannel(Channel* _channel) override;
  void setOutputChannel(Channel* _channel) override;
  void receiveMessage(Message* _message) override;
  void injectionAlgorithmResponse(
      Message* _message, InjectionAlgorithm::Response* _response) override;

  void sendFlit(u32 _port, Flit* _flit) override;
  void receiveFlit(u32 _port, Flit* _flit) override;
  void sendCredit(u32 _port, u32 _vc) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

 private:
  Channel* inputChannel_;
  Channel* outputChannel_;

  InjectionAlgorithm* injectionAlgorithm_;
  bool fixedMsgVc_;  // all pkts of a msg have same VC

  std::vector<InputQueue*> inputQueues_;
  Crossbar* crossbar_;
  CrossbarScheduler* crossbarScheduler_;
  Ejector* ejector_;

  std::vector<PacketReassembler*> packetReassemblers_;
  MessageReassembler* messageReassembler_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_INTERFACE_H_
