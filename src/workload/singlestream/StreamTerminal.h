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
#ifndef WORKLOAD_SINGLESTREAM_STREAMTERMINAL_H_
#define WORKLOAD_SINGLESTREAM_STREAMTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "traffic/MessageSizeDistribution.h"
#include "workload/Terminal.h"

class Application;

namespace SingleStream {

class Application;

class StreamTerminal : public Terminal {
 public:
  StreamTerminal(const std::string& _name, const Component* _parent,
                 u32 _id, const std::vector<u32>& _address, ::Application* _app,
                 Json::Value _settings);
  ~StreamTerminal();
  void processEvent(void* _event, s32 _type) override;
  void receiveMessage(Message* _message) override;
  void messageEnteredInterface(Message* _message) override;
  void messageExitedNetwork(Message* _message) override;
  f64 percentComplete() const;
  void start();
  void stop();

 private:
  void sendNextMessage();

  MessageSizeDistribution* messageSizeDistribution_;

  u32 trafficClass_;

  u32 numMessages_;
  u32 maxPacketSize_;  // flits
  u32 recvdMessages_;
  bool destReady_;
  bool destComplete_;

  // track state
  bool counting_;
  bool sending_;

  // message generator
  u64 lastSendTime_;
};

}  // namespace SingleStream

#endif  // WORKLOAD_SINGLESTREAM_STREAMTERMINAL_H_
