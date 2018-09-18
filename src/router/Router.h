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
#ifndef ROUTER_ROUTER_H_
#define ROUTER_ROUTER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <vector>

#include "architecture/PortedDevice.h"
#include "event/Component.h"
#include "metadata/MetadataHandler.h"
#include "types/CreditReceiver.h"
#include "types/CreditSender.h"
#include "types/FlitReceiver.h"
#include "types/FlitSender.h"

class Network;

#define ROUTER_ARGS const std::string&, const Component*, Network*, u32, \
    const std::vector<u32>&, u32, u32,                                  \
    const std::vector<std::tuple<u32, u32> >&, MetadataHandler*, Json::Value

class Router : public Component, public PortedDevice, public FlitSender,
               public FlitReceiver, public CreditSender, public CreditReceiver {
 public:
  Router(const std::string& _name, const Component* _parent, Network* _network,
         u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
         const std::vector<std::tuple<u32, u32> >& _protocolClassVcs,
         MetadataHandler* _metadataHandler, Json::Value _settings);
  virtual ~Router();

  // this is a router factory
  static Router* create(ROUTER_ARGS);

  // return the network of this router
  Network* network() const;

  // this must be called by all subclasses when a packet's head flit arrives
  //  on an input port.
  void packetArrival(u32 _port, Packet* _packet) const;

  // this must be called by all subclasses when a packet's head flit departs
  //  on an output port.
  void packetDeparture(u32 _port, Packet* _packet) const;

  virtual f64 congestionStatus(u32 _inputPort, u32 _inputVc,
                               u32 _outputPort, u32 _outputVc) const = 0;

 protected:
  Network* network_;
  const std::vector<std::tuple<u32, u32> > protocolClassVcs_;

 private:
  MetadataHandler* metadataHandler_;
};

#endif  // ROUTER_ROUTER_H_
