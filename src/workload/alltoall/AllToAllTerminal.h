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
#ifndef WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_
#define WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "traffic/distribution/DistributionTrafficPattern.h"
#include "traffic/size/MessageSizeDistribution.h"
#include "workload/Terminal.h"

class Application;

namespace AllToAll {

class Application;

class AllToAllTerminal : public Terminal {
 public:
  AllToAllTerminal(const std::string& _name, const Component* _parent,
                   u32 _id, const std::vector<u32>& _address,
                   ::Application* _app, Json::Value _settings);
  ~AllToAllTerminal();
  void processEvent(void* _event, s32 _type) override;
  f64 percentComplete() const;
  f64 requestInjectionRate() const;
  void start();

 protected:
  void handleDeliveredMessage(Message* _message) override;
  void handleReceivedMessage(Message* _message) override;

 private:
  void completeTracking(Message* _message);
  void completeLoggable(Message* _message);
  void checkCompletion();
  void sendNextRequest();
  void sendNextResponse(Message* _request);

  // traffic generation
  f64 requestInjectionRate_;
  u32 numIterations_;
  u32 maxPacketSize_;  // flits
  DistributionTrafficPattern* trafficPattern_;
  MessageSizeDistribution* messageSizeDistribution_;

  // state machine
  bool sendStalled_;

  // requests
  u32 requestTrafficClass_;

  // responses
  bool enableResponses_;
  u32 maxOutstandingTransactions_;  // 0=inf, >0=limit
  std::unordered_set<u64> outstandingTransactions_;
  u32 responseTrafficClass_;
  u64 requestProcessingLatency_;  // cycles

  // start time delay
  u64 delay_;

  // logging and message generation
  std::unordered_set<u32> transactionsToLog_;
  u32 requestsSent_;
  u32 loggableCompleteCount_;

  // these track the state of the iterations
  u32 sendIteration_;
  u32 recvIteration_;
  std::unordered_map<u32, std::unordered_set<u32> > iterationReceived_;
  bool sendWaitingForRecv_;
};

}  // namespace AllToAll

#endif  // WORKLOAD_ALLTOALL_ALLTOALLTERMINAL_H_
