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

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "metadata/MetadataHandler.h"
#include "network/Channel.h"
#include "types/FlitReceiver.h"
#include "types/FlitSender.h"
#include "types/CreditReceiver.h"
#include "types/CreditSender.h"

class Router : public Component, public FlitSender, public FlitReceiver,
               public CreditSender, public CreditReceiver {
 public:
  Router(const std::string& _name, const Component* _parent, u32 _numPorts,
         u32 _numVcs, const std::vector<u32>& _address,
         MetadataHandler* _metadataHandler, Json::Value _settings);
  virtual ~Router();

  u32 numPorts() const;
  u32 numVcs() const;
  const std::vector<u32>& getAddress() const;

  u32 vcIndex(u32 _port, u32 _vc) const;
  void vcIndexInv(u32 _index, u32* _port, u32* _vc) const;

  virtual void setInputChannel(u32 _port, Channel* _channel) = 0;
  virtual Channel* getInputChannel(u32 _port) = 0;
  virtual void setOutputChannel(u32 port, Channel* _channel) = 0;
  virtual Channel* getOutputChannel(u32 _port) = 0;

  // this should be called by all subclasses when a packet's head flit arrives
  //  on an input port.
  void packetArrival(Packet* _packet) const;

  // this returns creditCount/maxCredits (buffer availability)
  virtual f64 congestionStatus(u32 _port, u32 _vc) const;

 protected:
  const u32 numPorts_;
  const u32 numVcs_;
  const std::vector<u32> address_;
  MetadataHandler* metadataHandler_;
};

#endif  // ROUTER_ROUTER_H_
