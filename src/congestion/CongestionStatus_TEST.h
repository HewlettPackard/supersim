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
#ifndef CONGESTION_CONGESTIONSTATUS_TEST_H_
#define CONGESTION_CONGESTIONSTATUS_TEST_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "architecture/PortedDevice.h"
#include "congestion/CongestionStatus.h"
#include "event/Component.h"
#include "metadata/MetadataHandler.h"
#include "network/Channel.h"
#include "router/Router.h"

// this is a test class for implementing an router API for congestion status
//  tests
class CongestionTestRouter : public Router {
 public:
  CongestionTestRouter(
      const std::string& _name, const Component* _parent, Network* _network,
      u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
      MetadataHandler* _metadataHandler, Json::Value _settings);
  ~CongestionTestRouter();

  void setCongestionStatus(CongestionStatus* _congestionStatus);

  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) const override;
  void setOutputChannel(u32 _port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) const override;

  void receiveFlit(u32 _port, Flit* _flit) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void sendCredit(u32 _port, u32 _vc) override;
  void sendFlit(u32 _port, Flit* _flit) override;

  f64 congestionStatus(u32 _inputPort, u32 _inputVc, u32 _outputPort,
                       u32 _outputVc) const override;

 private:
  CongestionStatus* congestionStatus_;
  std::vector<Channel*> outputChannels_;
};

// this is a test class for altering the congestion status module under test
class CreditHandler : public Component {
 public:
  enum class Type {INCR, DECR};

  CreditHandler(const std::string& _name, const Component* _parent,
                CongestionStatus* _congestionStatus, PortedDevice* _device);
  ~CreditHandler();

  void setEvent(u32 _port, u32 _vc, u64 _time, u8 _epsilon,
                CreditHandler::Type _type);
  void processEvent(void* _event, s32 _type);

 private:
  struct Event {
    Type type;
    u32 port;
    u32 vc;
  };

  CongestionStatus* congestionStatus_;
  PortedDevice* device_;
};

// this is a test class for verifying proper congestion status
class StatusCheck : public Component {
 public:
  StatusCheck(const std::string& _name, const Component* _parent,
              CongestionStatus* _congestionStatus);
  ~StatusCheck();

  void setEvent(u64 _time, u8 _epsilon, u32 _inputPort, u32 _inputVc,
                u32 _outputPort, u32 _outputVc, f64 _expected);
  void processEvent(void* _event, s32 _type);

 private:
  struct Event {
    u32 inputPort;
    u32 inputVc;
    u32 outputPort;
    u32 outputVc;
    f64 exp;
  };

  CongestionStatus* congestionStatus_;
};

#endif  // CONGESTION_CONGESTIONSTATUS_TEST_H_
