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
#ifndef ROUTER_OUTPUTQUEUED_ROUTER_H_
#define ROUTER_OUTPUTQUEUED_ROUTER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <queue>
#include <string>
#include <tuple>
#include <vector>

#include "architecture/Crossbar.h"
#include "architecture/CrossbarScheduler.h"
#include "architecture/VcScheduler.h"
#include "congestion/CongestionSensor.h"
#include "event/Component.h"
#include "network/Channel.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"
#include "types/Credit.h"
#include "types/Flit.h"

class Network;

namespace OutputQueued {

class InputQueue;
class OutputQueue;
class Ejector;

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

  // only called on epsilon 0 from IQ
  void registerPacket(u32 _inputPort, u32 _inputVc, Flit* _headFlit,
                      u32 _outputPort, u32 _outputVc);

  // only called on epsilon 2 from OQ
  void newSpaceAvailable(u32 _outputPort, u32 _outputVc);

  void processEvent(void* _event, s32 _type) override;

 private:
  enum class CongestionMode {kOutput, kDownstream, kOutputAndDownstream};

  static CongestionMode parseCongestionMode(const std::string& _mode);

  // executes on epsilon 2
  void processTransfers(u32 _outputVcIdx);

  // executes on epsilon 1
  void transferPacket(u32 _outputVcIdx, Packet* _packet);

  const u32 transferLatency_;
  const CongestionMode congestionMode_;
  u32 creditSize_;

  u32 inputQueueDepth_;
  u32 outputQueueDepth_;  // U32_MAX for infinite

  // this vector is used to keep track of incoming port VC
  //  this is needed because of the hyperwarp symptom
  std::vector<u32> portVcs_;

  // this two vectors ensure packet buffer flow control and
  //  back-to-back cycle flit transmission
  std::vector<u64> expTimes_;
  std::vector<Packet*> expPackets_;

  std::vector<InputQueue*> inputQueues_;
  std::vector<RoutingAlgorithm*> routingAlgorithms_;
  CongestionSensor* congestionSensor_;

  std::vector<OutputQueue*> outputQueues_;
  std::vector<CrossbarScheduler*> outputCrossbarSchedulers_;
  std::vector<Crossbar*> outputCrossbars_;
  std::vector<Ejector*> ejectors_;

  std::vector<Channel*> inputChannels_;
  std::vector<Channel*> outputChannels_;

  // this is used to solve any starvation issues
  std::vector<std::queue<std::tuple<u32, u32, Flit*, u32, u32> > > waiting_;
};

}  // namespace OutputQueued

#endif  // ROUTER_OUTPUTQUEUED_ROUTER_H_
