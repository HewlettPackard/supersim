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
#include "network/uno/Network.h"

#include <cassert>
#include <cmath>

#include <tuple>

#include "interface/InterfaceFactory.h"
#include "network/uno/RoutingAlgorithmFactory.h"
#include "router/RouterFactory.h"

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
  std::vector<std::tuple<u32, u32> > trafficClassVcs;
  std::vector<::RoutingAlgorithmFactory*> routingAlgorithmFactories;
  for (u32 idx = 0; idx < _settings["traffic_classes"].size(); idx++) {
    u32 numVcs = _settings["traffic_classes"][idx]["num_vcs"].asUInt();
    u32 baseVc = routingAlgorithmFactories.size();
    trafficClassVcs.push_back(std::make_tuple(baseVc, numVcs));
    for (u32 vc = 0; vc < numVcs; vc++) {
      routingAlgorithmFactories.push_back(
          new RoutingAlgorithmFactory(
              baseVc, numVcs, concentration_,
              _settings["traffic_classes"][idx]["routing"]));
    }
  }

  // create the router
  router_ = RouterFactory::createRouter(
      "Router", this, 0, std::vector<u32>(), routerRadix, numVcs_,
      _metadataHandler, &routingAlgorithmFactories, _settings["router"]);

  // we don't need the routing algorithm factories anymore
  for (::RoutingAlgorithmFactory* raf : routingAlgorithmFactories) {
    delete raf;
  }
  routingAlgorithmFactories.clear();

  // create the interfaces and external channels
  interfaces_.resize(concentration_, nullptr);
  for (u32 id = 0; id < concentration_; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = InterfaceFactory::createInterface(
        interfaceName, this, id, {id}, numVcs_,
        trafficClassVcs, _settings["interface"]);
    interfaces_.at(id) = interface;

    // create the channels
    std::string inChannelName = "InChannel_" + std::to_string(id);
    Channel* inChannel = new Channel(inChannelName, this,
                                     _settings["external_channel"]);
    externalChannels_.push_back(inChannel);
    std::string outChannelName = "OutChannel_" + std::to_string(id);
    Channel* outChannel = new Channel(outChannelName, this,
                                      _settings["external_channel"]);
    externalChannels_.push_back(outChannel);

    // link interfaces to router via channels
    router_->setInputChannel(id, inChannel);
    router_->setOutputChannel(id, outChannel);
    interface->setInputChannel(0, outChannel);
    interface->setOutputChannel(0, inChannel);
  }
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
