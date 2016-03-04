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
#ifndef ROUTER_INPUTOUTPUTQUEUED_ROUTER_H_
#define ROUTER_INPUTOUTPUTQUEUED_ROUTER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/Channel.h"
#include "network/RoutingAlgorithm.h"
#include "network/RoutingAlgorithmFactory.h"
#include "router/common/CongestionStatus.h"
#include "router/common/Crossbar.h"
#include "router/common/CrossbarScheduler.h"
#include "router/common/VcScheduler.h"
#include "router/Router.h"
#include "types/Credit.h"
#include "types/Flit.h"

namespace InputOutputQueued {

class InputQueue;
class OutputQueue;
class Ejector;

class Router : public ::Router {
 public:
  Router(const std::string& _name, const Component* _parent,
         const std::vector<u32>& _address,
         RoutingAlgorithmFactory* _routingAlgorithmFactory,
         Json::Value _settings);
  ~Router();

  // Network
  void setInputChannel(u32 _index, Channel* _channel) override;
  void setOutputChannel(u32 _index, Channel* _channel) override;

  void receiveFlit(u32 _port, Flit* _flit) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void sendCredit(u32 _port, u32 _vc) override;
  void sendFlit(u32 _port, Flit* _flit) override;

  f64 congestionStatus(u32 _vcIdx) const override;

 private:
  std::vector<InputQueue*> inputQueues_;
  std::vector<RoutingAlgorithm*> routingAlgorithms_;
  CongestionStatus* congestionStatus_;
  Crossbar* crossbar_;
  CrossbarScheduler* crossbarScheduler_;
  VcScheduler* vcScheduler_;

  std::vector<OutputQueue*> outputQueues_;
  std::vector<CrossbarScheduler*> outputCrossbarSchedulers_;
  std::vector<Crossbar*> outputCrossbars_;
  std::vector<Ejector*> ejectors_;

  std::vector<Channel*> inputChannels_;
  std::vector<Channel*> outputChannels_;
};

}  // namespace InputOutputQueued

#endif  // ROUTER_INPUTOUTPUTQUEUED_ROUTER_H_
