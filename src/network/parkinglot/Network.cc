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
#include "network/parkinglot/Network.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

namespace ParkingLot {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // attributes
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);
  u32 routerRadix = concentration_ + 2;
  inputPort_ = _settings["input_port"].asUInt();
  assert(inputPort_ < routerRadix);
  outputPort_ = _settings["output_port"].asUInt();
  assert(outputPort_ < routerRadix);
  assert(outputPort_ != inputPort_);
  u32 routers = _settings["routers"].asUInt();
  assert(routers > 0);
  u32 interfaces = concentration_ * routers + 1;

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create the routers
  routers_.resize(routers, nullptr);
  for (u32 router = 0; router < routers; router++) {
    std::string routerName = "Router_" + std::to_string(router);
    dbgprintf("made %s", routerName.c_str());

    // use the router factory to create a router
    routers_.at(router) = Router::create(
        routerName, this, this, router, {router}, routerRadix, numVcs_,
        protocolClassVcs_, _metadataHandler, _settings["router"]);
  }

  // link routers via channels
  for (u32 router = 0; router < routers - 1; router++) {
    // create the channel
    std::string channelName =
        "Channel_"  + std::to_string(router) + "-" + std::to_string(router + 1);
    Channel* channel = new Channel(channelName, this, numVcs_,
                                   _settings["internal_channel"]);
    internalChannels_.push_back(channel);

    // link routers
    Router* sourceRouter = routers_.at(router);
    assert(sourceRouter->getOutputChannel(outputPort_) == nullptr);
    sourceRouter->setOutputChannel(outputPort_, channel);

    Router* destinationRouter = routers_.at(router + 1);
    assert(destinationRouter->getInputChannel(inputPort_) == nullptr);
    destinationRouter->setInputChannel(inputPort_, channel);

    dbgprintf("connect %s:%u to %s:%u with %s",
              sourceRouter->name().c_str(), outputPort_,
              destinationRouter->name().c_str(), inputPort_,
              channelName.c_str());
  }

  // create injection interfaces, link to routers
  interfaces_.resize(interfaces, nullptr);
  u32 interfaceId = 0;
  for (u32 routerId = 0; routerId < routers; routerId++) {
    for (u32 term = 0; term < concentration_; term++) {
      // find right router port
      u32 routerPort = term;
      if (inputPort_ <= term) {
        routerPort++;
      }
      if (outputPort_ <= term) {
        routerPort++;
      }

      // create an interface name
      std::string interfaceName = "Interface_" + std::to_string(interfaceId);
      dbgprintf("made %s", interfaceName.c_str());

      // create the interface
      Interface* interface = Interface::create(
          interfaceName, this, interfaceId, {routerId, term}, numVcs_,
          protocolClassVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceId) = interface;

      std::string channelName = "InjChannel_" + std::to_string(interfaceId) +
                                "-to-" + std::to_string(routerId);
      Channel* channel = new Channel(channelName, this, numVcs_,
                                     _settings["external_channel"]);
      externalChannels_.push_back(channel);

      // link with router
      Router* router = routers_.at(routerId);
      router->setInputChannel(routerPort, channel);
      interface->setOutputChannel(0, channel);

      dbgprintf("connect %s to %s:%u with %s",
                interfaceName.c_str(),
                router->name().c_str(), routerPort,
                channelName.c_str());


      // go to next interface ID
      interfaceId++;
    }
  }

  // create ejection interface, link to router
  {
    // create an interface name
    std::string interfaceName = "Interface_" + std::to_string(interfaceId);
    dbgprintf("made %s", interfaceName.c_str());

    // create the interface
    u32 routerId = routers - 1;
    u32 term = interfaces - 1;
    Interface* interface = Interface::create(
        interfaceName, this, interfaceId, {routerId, term}, numVcs_,
        protocolClassVcs_, _metadataHandler, _settings["interface"]);
    interfaces_.at(interfaceId) = interface;

    std::string channelName = "EjChannel_" + std::to_string(routerId) +
                              "-to-" + std::to_string(interfaceId);
    Channel* channel = new Channel(channelName, this, numVcs_,
                                   _settings["external_channel"]);
    externalChannels_.push_back(channel);

    // link with router
    Router* router = routers_.at(routerId);
    router->setOutputChannel(outputPort_, channel);
    interface->setInputChannel(0, channel);

    dbgprintf("connect %s:%u to %s with %s",
              router->name().c_str(), outputPort_,
              interfaceName.c_str(),
              channelName.c_str());
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  // delete routers
  for (u32 id = 0; id < routers_.size(); id++) {
    delete routers_.at(id);
  }

  // delete interfaces
  for (u32 id = 0; id < interfaces_.size(); id++) {
    delete interfaces_.at(id);
  }

  // delete channels
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    delete *it;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    delete *it;
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
      outputPort_, info.settings);
}

u32 Network::numRouters() const {
  return routers_.size();
}

u32 Network::numInterfaces() const {
  return interfaces_.size();
}

Router* Network::getRouter(u32 _id) const {
  return routers_.at(_id);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1, _id);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return _address->at(0);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  _address->resize(1, _id);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return _address->at(0);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  u32 numr = routers_.size();
  u32 src = translateRouterAddressToId(_source);
  u32 myr = src / concentration_;
  u32 minHops = numr - myr;
  return minHops;
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    _channels->push_back(*it);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    _channels->push_back(*it);
  }
}

}  // namespace ParkingLot

registerWithObjectFactory("parking_lot", ::Network,
                          ParkingLot::Network, NETWORK_ARGS);
