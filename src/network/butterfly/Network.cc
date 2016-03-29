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
#include "network/butterfly/Network.h"

#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include "interface/InterfaceFactory.h"
#include "network/butterfly/InjectionAlgorithmFactory.h"
#include "network/butterfly/RoutingAlgorithmFactory.h"
#include "router/RouterFactory.h"

namespace Butterfly {

Network::Network(const std::string& _name, const Component* _parent,
                 Json::Value _settings)
    : ::Network(_name, _parent, _settings) {
  // radix and stages
  routerRadix_ = _settings["router"]["num_ports"].asUInt();
  assert(routerRadix_ >= 2);
  numStages_ = _settings["stages"].asUInt();
  assert(numStages_ >= 1);
  stageWidth_ = (u32)pow(routerRadix_, numStages_ - 1);

  // num VCs for subcomponents
  _settings["router"]["num_vcs"] = Json::Value(numVcs_);
  _settings["interface"]["num_vcs"] = Json::Value(numVcs_);

  // create the routers
  routers_.resize(numStages_);
  for (u32 stage = 0; stage < numStages_; stage++) {
    routers_.at(stage).resize(stageWidth_, nullptr);

    for (u32 column = 0; column < stageWidth_; column++) {
      // create the router name
      std::string rname = "Router_" + std::to_string(stage) + "-" +
          std::to_string(column);

      // create a routing algorithm factory to give to the router
      RoutingAlgorithmFactory* routingAlgorithmFactory =
          new RoutingAlgorithmFactory(numVcs_, routerRadix_, numStages_, stage,
                                      _settings["routing"]);

      // create the router
      routers_.at(stage).at(column) = RouterFactory::createRouter(
          rname, this, std::vector<u32>({stage, column}),
          routingAlgorithmFactory, _settings["router"]);
      delete routingAlgorithmFactory;
    }
  }

  // create internal channels, link routers via channels
  for (u32 cStage = 0; cStage < numStages_ - 1; cStage++) {
    u32 cBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - cStage);
    u32 nStage = cStage + 1;
    u32 nBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - nStage);
    for (u32 cColumn = 0; cColumn < stageWidth_; cColumn++) {
      Router* sourceRouter = routers_.at(cStage).at(cColumn);
      u32 cBaseOffset = (cColumn / cBaseUnit) * cBaseUnit;
      u32 cBaseIndex = cColumn % cBaseUnit;
      for (u32 cOutputPort = 0; cOutputPort < routerRadix_; cOutputPort++) {
        u32 nColumn = cBaseOffset + (cBaseIndex % nBaseUnit) +
            (cOutputPort * nBaseUnit);
        Router* destinationRouter = routers_.at(nStage).at(nColumn);
        u32 nInputPort = cBaseIndex / nBaseUnit;  // cBaseIndex / routerRadix_;

        // create channel
        std::string chname = "Channel_" +
            strop::vecString<u32>(sourceRouter->getAddress()) + "-to-" +
            strop::vecString<u32>(destinationRouter->getAddress());
        Channel* channel = new Channel(chname, this,
                                       _settings["internal_channel"]);
        internalChannels_.push_back(channel);

        // need to connect
        sourceRouter->setOutputChannel(cOutputPort, channel);
        destinationRouter->setInputChannel(nInputPort, channel);
      }
    }
  }

  // create an injection algorithm factory to give to the interfaces
  InjectionAlgorithmFactory* injectionAlgorithmFactory =
      new InjectionAlgorithmFactory(numVcs_, _settings["routing"]);

  // create the interfaces and external channels
  u32 ifaces = routerRadix_ * stageWidth_;
  interfaces_.resize(ifaces, nullptr);
  for (u32 id = 0; id < ifaces; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    Interface* interface = InterfaceFactory::createInterface(
        interfaceName, this, id, injectionAlgorithmFactory,
        _settings["interface"]);
    interfaces_.at(id) = interface;

    // get references to the routers
    u32 routerIndex = id / routerRadix_;
    u32 routerPort = id % routerRadix_;
    Router* inputRouter = routers_.at(0).at(routerIndex);
    Router* outputRouter = routers_.at(numStages_-1).at(routerIndex);

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
    interface->setOutputChannel(inChannel);
    inputRouter->setInputChannel(routerPort, inChannel);
    interface->setInputChannel(outChannel);
    outputRouter->setOutputChannel(routerPort, outChannel);
  }

  delete injectionAlgorithmFactory;
}

Network::~Network() {
  for (u32 s = 0; s < numStages_; s++) {
    for (u32 i = 0; i < stageWidth_; i++) {
      delete routers_.at(s).at(i);
    }
  }
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    delete channel;
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* channel = *it;
    delete channel;
  }
}

u32 Network::numRouters() const {
  return stageWidth_ * numStages_;
}

u32 Network::numInterfaces() const {
  return routerRadix_ * stageWidth_;
}

Router* Network::getRouter(u32 _id) const {
  u32 stage = _id / numStages_;
  u32 index = _id % numStages_;
  return routers_.at(stage).at(index);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateIdToAddress(u32 _id, std::vector<u32>* _address) const {
  _address->resize(numStages_);
  // work in reverse for little endian format
  for (u32 exp = 0, row = numStages_ - 1; exp < numStages_; exp++, row--) {
    u32 divisor = (u32)pow(routerRadix_, row);
    _address->at(exp) = _id / divisor;
    _id %= divisor;
  }
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* channel = *it;
    _channels->push_back(channel);
  }
}

}  // namespace Butterfly
