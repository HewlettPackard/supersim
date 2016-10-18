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
#ifndef WORKLOAD_STRESSTEST_BLASTTERMINAL_H_
#define WORKLOAD_STRESSTEST_BLASTTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "traffic/TrafficPattern.h"
#include "workload/Terminal.h"

class Application;

namespace StressTest {

class Application;

class BlastTerminal : public Terminal {
 public:
  BlastTerminal(const std::string& _name, const Component* _parent,
                u32 _id, const std::vector<u32>& _address,
                ::Application* _app, Json::Value _settings);
  ~BlastTerminal();
  void processEvent(void* _event, s32 _type) override;
  void receiveMessage(Message* _message) override;
  void messageEnteredInterface(Message* _message) override;
  void messageExitedNetwork(Message* _message) override;
  f64 percentComplete() const;
  void stopWarming();
  void startLogging();
  void stopLogging();
  void stopSending();

 private:
  // WARMING = sending and monitoring outstanding flits to determine if warm/sat
  // LOGGING = sending messages marked to be logged
  // BLABBING = sending messages not marked to be logged
  // DRAINING = not sending messages
  enum class Fsm : u8 {WARMING = 0, WARM_BLABBING = 1, LOGGING = 2,
      LOG_BLABBING = 3 , DRAINING = 4};

  void warm(bool _saturated);
  void complete();
  void done();
  void sendNextMessage();

  TrafficPattern* trafficPattern_;

  u32 trafficClass_;

  // state machine
  Fsm fsm_;

  // messages
  u32 numMessages_;
  u32 minMessageSize_;  // flits
  u32 maxMessageSize_;  // flits
  u32 maxPacketSize_;  // flits

  // warmup/saturation detector
  u32 warmupInterval_;  // messages received
  u32 warmupFlitsReceived_;
  u32 warmupWindow_;
  u32 maxWarmupAttempts_;
  u32 warmupAttempts_;
  std::vector<u64> enrouteSampleTimes_;
  std::vector<u64> enrouteSampleValues_;
  u32 enrouteSamplePos_;
  u32 fastFailSample_;

  // message generator
  u64 lastSendTime_;

  // logging and message generation
  std::unordered_set<u32> messagesToLog_;
  u32 loggableEnteredCount_;
  u32 loggableExitedCount_;
};

}  // namespace StressTest

#endif  // WORKLOAD_STRESSTEST_BLASTTERMINAL_H_
