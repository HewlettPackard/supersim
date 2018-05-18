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
#ifndef ROUTER_INPUTQUEUED_ROUTER_H_
#define ROUTER_INPUTQUEUED_ROUTER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "architecture/Crossbar.h"
#include "architecture/CrossbarScheduler.h"
#include "architecture/VcScheduler.h"
#include "congestion/CongestionSensor.h"
#include "event/Component.h"
#include "network/Channel.h"
#include "routing/RoutingAlgorithm.h"
#include "router/Router.h"
#include "types/Credit.h"
#include "types/Flit.h"

class Network;

namespace InputQueued {

class InputQueue;
class OutputQueue;

class Router : public ::Router {
 public:
  Router(const std::string& _name, const Component* _parent, Network* _network,
         u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
         const std::vector<std::tuple<u32, u32> >& _protocolClassVcs,
         MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Router();

  // Network
  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) const override;
  void setOutputChannel(u32 _port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) const override;

  // override to initialize credits
  void initialize() override;

  void receiveFlit(u32 _port, Flit* _flit) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void sendCredit(u32 _port, u32 _vc) override;
  void sendFlit(u32 _port, Flit* _flit) override;

  f64 congestionStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                       u32 _outputVc) const override;

 private:
  enum class CongestionMode {kOutput, kDownstream};

  static CongestionMode parseCongestionMode(const std::string& _mode);

  const CongestionMode congestionMode_;
  u32 creditSize_;
  u32 inputQueueDepth_;
  // input queue tailoring
  bool inputQueueTailored_;
  f64 inputQueueMult_;
  u32 inputQueueMax_;
  u32 inputQueueMin_;

  std::vector<InputQueue*> inputQueues_;
  std::vector<RoutingAlgorithm*> routingAlgorithms_;
  CongestionSensor* congestionSensor_;
  Crossbar* crossbar_;
  CrossbarScheduler* crossbarScheduler_;
  VcScheduler* vcScheduler_;
  std::vector<OutputQueue*> outputQueues_;

  std::vector<Channel*> inputChannels_;
  std::vector<Channel*> outputChannels_;
};

}  // namespace InputQueued

#endif  // ROUTER_INPUTQUEUED_ROUTER_H_
