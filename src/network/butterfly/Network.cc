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
#include "network/butterfly/Network.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/butterfly/util.h"

namespace Butterfly {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // radix and stages
  routerRadix_ = _settings["radix"].asUInt();
  assert(routerRadix_ >= 2);
  numStages_ = _settings["stages"].asUInt();
  assert(numStages_ >= 1);
  stageWidth_ = (u32)pow(routerRadix_, numStages_ - 1);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the routers
  routers_.resize(stageWidth_ * numStages_, nullptr);
  for (u32 stage = 0; stage < numStages_; stage++) {
    tmpStage_ = stage;
    for (u32 column = 0; column < stageWidth_; column++) {
      // create the router name
      std::string rname = "Router_" + std::to_string(stage) + "-" +
                          std::to_string(column);

      // create the router
      u32 routerId = stage * stageWidth_ + column;
      routers_.at(routerId) = Router::create(
          rname, this, this, routerId, {stage, column}, routerRadix_, numVcs_,
          protocolClassVcs_, _metadataHandler, _settings["router"]);
    }
  }

  // create internal channels, link routers via channels
  for (u32 cStage = 0; cStage < numStages_ - 1; cStage++) {
    u32 cBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - cStage);
    u32 nStage = cStage + 1;
    u32 nBaseUnit = (u32)pow(routerRadix_, numStages_ - 1 - nStage);
    for (u32 cColumn = 0; cColumn < stageWidth_; cColumn++) {
      u32 sourceId = cStage * stageWidth_ + cColumn;
      Router* sourceRouter = routers_.at(sourceId);
      u32 cBaseOffset = (cColumn / cBaseUnit) * cBaseUnit;
      u32 cBaseIndex = cColumn % cBaseUnit;
      for (u32 cOutputPort = 0; cOutputPort < routerRadix_; cOutputPort++) {
        u32 nColumn = cBaseOffset + (cBaseIndex % nBaseUnit) +
                      (cOutputPort * nBaseUnit);
        u32 destinationId = nStage * stageWidth_ + nColumn;
        Router* destinationRouter = routers_.at(destinationId);
        u32 nInputPort = cBaseIndex / nBaseUnit;  // cBaseIndex / routerRadix_;

        // create channel
        std::string chname =
            "Channel_" + strop::vecString<u32>(sourceRouter->address(), '-') +
            "-to-" + strop::vecString<u32>(destinationRouter->address(), '-');
        Channel* channel = new Channel(chname, this, numVcs_,
                                       _settings["internal_channel"]);
        internalChannels_.push_back(channel);

        // need to connect
        sourceRouter->setOutputChannel(cOutputPort, channel);
        destinationRouter->setInputChannel(nInputPort, channel);
      }
    }
  }

  // create the interfaces and external channels
  u32 ifaces = routerRadix_ * stageWidth_;
  interfaces_.resize(ifaces, nullptr);
  for (u32 id = 0; id < ifaces; id++) {
    // create the interface
    std::string interfaceName = "Interface_" + std::to_string(id);
    std::vector<u32> interfaceAddress;
    translateInterfaceIdToAddress(id, &interfaceAddress);
    Interface* interface = Interface::create(
        interfaceName, this, id, interfaceAddress, numVcs_, protocolClassVcs_,
        _metadataHandler, _settings["interface"]);
    interfaces_.at(id) = interface;

    // get references to the routers
    u32 routerIndex = id / routerRadix_;
    u32 routerPort = id % routerRadix_;
    u32 inputRouterId = 0 * stageWidth_ + routerIndex;
    u32 outputRouterId = (numStages_ - 1) * stageWidth_ + routerIndex;
    Router* inputRouter = routers_.at(inputRouterId);
    Router* outputRouter = routers_.at(outputRouterId);

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
    interface->setOutputChannel(0, inChannel);
    inputRouter->setInputChannel(routerPort, inChannel);
    interface->setInputChannel(0, outChannel);
    outputRouter->setOutputChannel(routerPort, outChannel);
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  for (auto it = routers_.begin(); it != routers_.end(); ++it) {
    Router* router = *it;
    delete router;
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

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _inputPort, u32 _inputVc, const std::string& _name,
     const Component* _parent, Router* _router) {
  // get the info
  const Network::RoutingAlgorithmInfo& info =
      routingAlgorithmInfo_.at(_inputVc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(
      _name, _parent, _router, info.baseVc, info.numVcs, _inputPort, _inputVc,
      routerRadix_, numStages_, tmpStage_, info.settings);
}

u32 Network::numRouters() const {
  return stageWidth_ * numStages_;
}

u32 Network::numInterfaces() const {
  return routerRadix_ * stageWidth_;
}

Router* Network::getRouter(u32 _id) const {
  return routers_.at(_id);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Butterfly::translateInterfaceIdToAddress(
      routerRadix_, numStages_, stageWidth_, _id, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return Butterfly::translateInterfaceAddressToId(
      routerRadix_, numStages_, stageWidth_, _address);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Butterfly::translateRouterIdToAddress(
      routerRadix_, numStages_, stageWidth_, _id, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return Butterfly::translateRouterAddressToId(
      routerRadix_, numStages_, stageWidth_, _address);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return Butterfly::computeMinimalHops(numStages_);
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

registerWithObjectFactory("butterfly", ::Network,
                          Butterfly::Network, NETWORK_ARGS);
