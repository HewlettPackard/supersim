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

#include "interface/InterfaceFactory.h"
#include "network/uno/InjectionAlgorithmFactory.h"
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

  // create a routing algorithm factory to give to the routers
  RoutingAlgorithmFactory* routingAlgorithmFactory =
      new RoutingAlgorithmFactory(numVcs_, concentration_,
                                  _settings["routing"]);

  // create the router
  router_ = RouterFactory::createRouter(
      "Router", this, routerRadix, numVcs_, std::vector<u32>(),
      _metadataHandler, routingAlgorithmFactory, _settings["router"]);
  delete routingAlgorithmFactory;

  // create an injection algorithm factory to give to the interfaces
  InjectionAlgorithmFactory* injectionAlgorithmFactory =
      new InjectionAlgorithmFactory(numVcs_, _settings["routing"]);

  // create the interfaces and external channels
  interfaces_.resize(concentration_, nullptr);
  for (u32 id = 0; id < concentration_; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = InterfaceFactory::createInterface(
        interfaceName, this, numVcs_, id, injectionAlgorithmFactory,
        _settings["interface"]);
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
    interface->setInputChannel(outChannel);
    interface->setOutputChannel(inChannel);
  }

  delete injectionAlgorithmFactory;
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

void Network::translateTerminalIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1);
  _address->at(0) = _id;
}

u32 Network::translateTerminalAddressToId(
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
