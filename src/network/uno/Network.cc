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
#include "network/uno/Network.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/uno/RoutingAlgorithm.h"

namespace Uno {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // dimensions and concentration
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);
  dbgprintf("concentration_ = %u", concentration_);

  // router radix
  u32 routerRadix = concentration_;

  // parse the traffic classes description
  loadTrafficClassInfo(_settings["traffic_classes"]);

  // create the router
  router_ = Router::create(
      "Router", this, this, 0, std::vector<u32>(), routerRadix, numVcs_,
      _metadataHandler, _settings["router"]);

  // create the interfaces and external channels
  interfaces_.resize(concentration_, nullptr);
  for (u32 id = 0; id < concentration_; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = Interface::create(
        interfaceName, this, id, {id}, numVcs_,
        trafficClassVcs_, _settings["interface"]);
    interfaces_.at(id) = interface;

    // create the channels
    std::string inChannelName = "InChannel_" + std::to_string(id);
    Channel* inChannel = new Channel(inChannelName, this, numVcs_,
                                     _settings["external_channel"]);
    externalChannels_.push_back(inChannel);
    std::string outChannelName = "OutChannel_" + std::to_string(id);
    Channel* outChannel = new Channel(outChannelName, this, numVcs_,
                                      _settings["external_channel"]);
    externalChannels_.push_back(outChannel);

    // link interfaces to router via channels
    router_->setInputChannel(id, inChannel);
    router_->setOutputChannel(id, outChannel);
    interface->setInputChannel(0, outChannel);
    interface->setOutputChannel(0, inChannel);
  }

  // clear the traffic class info
  clearTrafficClassInfo();
}

Network::~Network() {
  delete router_;
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    delete channel;
  }
}

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _vc, u32 _port, const std::string& _name, const Component* _parent,
     Router* _router) {
  // get the info
  const Network::RoutingAlgorithmInfo& info = routingAlgorithmInfo_.at(_vc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(_name, _parent, _router, info.baseVc,
                                  info.numVcs, concentration_, info.settings);
}

u32 Network::numRouters() const {
  return 1;
}

u32 Network::numInterfaces() const {
  return concentration_;
}

Router* Network::getRouter(u32 _id) const {
  assert(_id == 0);
  return router_;
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = _id;
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return _address->at(0);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = 0;
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return 0;
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
}

}  // namespace Uno

registerWithFactory("uno", ::Network,
                    Uno::Network, NETWORK_ARGS);
