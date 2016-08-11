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
#ifndef ROUTER_COMMON_CONGESTION_CONGESTIONSTATUS_TEST_H_
#define ROUTER_COMMON_CONGESTION_CONGESTIONSTATUS_TEST_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "metadata/MetadataHandler.h"
#include "network/Channel.h"
#include "router/Router.h"
#include "router/common/congestion/CongestionStatus.h"

class CongestionTestRouter : public Router {
 public:
  CongestionTestRouter(
      const std::string& _name, const Component* _parent,
      const std::vector<u32>& _address, MetadataHandler* _metadataHandler,
      Json::Value _settings);
  ~CongestionTestRouter();

  void setCongestionStatus(CongestionStatus* _congestionStatus);

  void setInputChannel(u32 _port, Channel* _channel) override;
  Channel* getInputChannel(u32 _port) override;
  void setOutputChannel(u32 _port, Channel* _channel) override;
  Channel* getOutputChannel(u32 _port) override;

  void receiveFlit(u32 _port, Flit* _flit) override;
  void receiveCredit(u32 _port, Credit* _credit) override;

  void sendCredit(u32 _port, u32 _vc) override;
  void sendFlit(u32 _port, Flit* _flit) override;

  f64 congestionStatus(u32 _port, u32 _vc) const override;

 private:
  CongestionStatus* congestionStatus_;
  std::vector<Channel*> outputChannels_;
};

class CreditHandler : public Component {
 public:
  CreditHandler(const std::string& _name, const Component* _parent,
                CongestionStatus* _congestionStatus, Router* _router);
  ~CreditHandler();

  void setEvent(u32 _port, u32 _vc, u64 _time, u8 _epsilon, s32 _type);
  void processEvent(void* _event, s32 _type);

 private:
  CongestionStatus* congestionStatus_;
  Router* router_;
};

class StatusCheck : public Component {
 public:
  StatusCheck(const std::string& _name, const Component* _parent,
              CongestionStatus* _congestionStatus);
  ~StatusCheck();

  struct Event {
    u32 port;
    u32 vc;
    f64 exp;
  };

  void setEvent(u64 _time, u8 _epsilon, u32 _port, u32 _vc, f64 _expected);
  void processEvent(void* _event, s32 _type);

 private:
  CongestionStatus* congestionStatus_;
};

#endif  // ROUTER_COMMON_CONGESTION_CONGESTIONSTATUS_TEST_H_
