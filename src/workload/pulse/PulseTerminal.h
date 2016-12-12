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
#ifndef WORKLOAD_PULSE_PULSETERMINAL_H_
#define WORKLOAD_PULSE_PULSETERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "traffic/TrafficPattern.h"
#include "workload/Terminal.h"

class Application;

namespace Pulse {

class Application;

class PulseTerminal : public Terminal {
 public:
  PulseTerminal(const std::string& _name, const Component* _parent,
                u32 _id, const std::vector<u32>& _address,
                ::Application* _app, Json::Value _settings);
  ~PulseTerminal();
  void processEvent(void* _event, s32 _type) override;
  void receiveMessage(Message* _message) override;
  void messageEnteredInterface(Message* _message) override;
  void messageExitedNetwork(Message* _message) override;
  f64 percentComplete() const;
  void start();

 private:
  void sendNextMessage();

  TrafficPattern* trafficPattern_;

  u32 trafficClass_;

  // messages
  u32 numMessages_;
  u32 minMessageSize_;  // flits
  u32 maxMessageSize_;  // flits
  u32 maxPacketSize_;  // flits
  bool fakeResponses_;  // sends 1 flit msgs 50% of time

  // start time delay
  u64 delay_;

  // message generator
  u64 lastSendTime_;

  // counter
  u32 loggableEnteredCount_;
  u32 loggableExitedCount_;
};

}  // namespace Pulse

#endif  // WORKLOAD_PULSE_PULSETERMINAL_H_
