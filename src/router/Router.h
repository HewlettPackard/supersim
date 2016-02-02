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
#ifndef ROUTER_ROUTER_H_
#define ROUTER_ROUTER_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/Channel.h"
#include "types/FlitReceiver.h"
#include "types/ControlReceiver.h"

class Router : public Component, public FlitReceiver, public ControlReceiver {
 public:
  Router(const std::string& _name, const Component* _parent,
         Json::Value _settings);
  virtual ~Router();

  u32 numPorts() const;
  u32 numVcs() const;
  void setAddress(const std::vector<u32>& _address);
  const std::vector<u32>& getAddress() const;

  virtual void setInputChannel(u32 _port, Channel* _channel) = 0;
  virtual void setOutputChannel(u32 port, Channel* _channel) = 0;

 protected:
  const u32 numPorts_;
  const u32 numVcs_;
  std::vector<u32> address_;
};

#endif  // ROUTER_ROUTER_H_
