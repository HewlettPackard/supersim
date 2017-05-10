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
#ifndef WORKLOAD_BLAST_BLASTTERMINAL_H_
#define WORKLOAD_BLAST_BLASTTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include "event/Component.h"
#include "traffic/continuous/ContinuousTrafficPattern.h"
#include "traffic/size/MessageSizeDistribution.h"
#include "workload/Terminal.h"

class Application;

namespace Blast {

class Application;

class BlastTerminal : public Terminal {
 public:
  BlastTerminal(const std::string& _name, const Component* _parent,
                u32 _id, const std::vector<u32>& _address,
                ::Application* _app, Json::Value _settings);
  ~BlastTerminal();
  void processEvent(void* _event, s32 _type) override;
  f64 percentComplete() const;
  f64 requestInjectionRate() const;
  void stopWarming();
  void startLogging();
  void stopLogging();
  void stopSending();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;

 private:
  // WARMING = sending and monitoring outstanding flits to determine if warm/sat
  // LOGGING = sending messages marked to be logged
  // BLABBING = sending messages not marked to be logged
  // DRAINING = not sending messages
  enum class Fsm : u8 {WARMING = 0, WARM_BLABBING = 1, LOGGING = 2,
      LOG_BLABBING = 3 , DRAINING = 4};

  void warmDetector(Message* _message);
  void warm(bool _saturated);
  void complete();
  void done();
  void completeTracking(Message* _message);
  void completeLoggable(Message* _message);
  void sendNextRequest();
  void sendNextResponse(Message* _request);

  // state machine
  Fsm fsm_;
  bool sendStalled_;

  // traffic generation
  f64 requestInjectionRate_;
  u32 numTransactions_;
  u32 maxPacketSize_;  // flits
  ContinuousTrafficPattern* trafficPattern_;
  MessageSizeDistribution* messageSizeDistribution_;

  // requests
  u32 requestTrafficClass_;

  // responses
  bool enableResponses_;
  u32 maxOutstandingTransactions_;  // 0=inf, >0=limit
  std::unordered_set<u64> outstandingTransactions_;
  u32 responseTrafficClass_;
  u64 requestProcessingLatency_;  // cycles

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

  // logging and message generation
  std::unordered_set<u32> transactionsToLog_;
  u32 loggableCompleteCount_;
};

}  // namespace Blast

#endif  // WORKLOAD_BLAST_BLASTTERMINAL_H_
