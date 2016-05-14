/*
 * Copyright 2016 Ashish Chaudhari, Franky Romero, Nehal Bhandari, Wasam Altoyan
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

#include "network/slimfly/Network.h"

#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include "interface/InterfaceFactory.h"
#include "network/slimfly/InjectionAlgorithmFactory.h"
#include "network/slimfly/RoutingAlgorithmFactory.h"
#include "router/RouterFactory.h"
#include "util/DimensionIterator.h"

namespace SlimFly {

Network::Network(const std::string& _name, const Component* _parent,
                 Json::Value _settings)
    : ::Network(_name, _parent, _settings) {
  // dimensions and concentration
  assert(_settings["dimension_widths"].isArray());
  dimensions_ = _settings["dimension_widths"].size();
  assert(_settings["dimension_weights"].size() == dimensions_);
  dimensionWidths_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWidths_.at(i) = _settings["dimension_widths"][i].asUInt();
    assert(dimensionWidths_.at(i) >= 2);
  }
  dimensionWeights_.resize(dimensions_);
  for (u32 i = 0; i < dimensions_; i++) {
    dimensionWeights_.at(i) = _settings["dimension_weights"][i].asUInt();
    assert(dimensionWeights_.at(i) >= 1);
  }
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);
  dbgprintf("dimensions_ = %u", dimensions_);
  dbgprintf("dimensionWidths_ = %s",
            strop::vecString<u32>(dimensionWidths_, '-').c_str());
  dbgprintf("dimensionWeights_ = %s",
            strop::vecString<u32>(dimensionWeights_, '-').c_str());
  dbgprintf("concentration_ = %u", concentration_);

  // router radix
  assert(_settings["router"].isMember("num_ports") == false);
  u32 routerRadix = concentration_;
  for (u32 i = 0; i < dimensions_; i++) {
    routerRadix += ((dimensionWidths_.at(i) - 1) * dimensionWeights_.at(i));
  }
  _settings["router"]["num_ports"] = Json::Value(routerRadix);
  _settings["router"]["num_vcs"] = Json::Value(numVcs_);
  _settings["interface"]["num_vcs"] = Json::Value(numVcs_);

  // create a routing algorithm factory to give to the routers
  RoutingAlgorithmFactory* routingAlgorithmFactory =
      new SlimFly::RoutingAlgorithmFactory(
          numVcs_, dimensionWidths_, dimensionWeights_,
          concentration_, _settings["routing"]);

  // setup a router iterator for looping over the router dimensions
  DimensionIterator routerIterator(dimensionWidths_);
  std::vector<u32> routerAddress(dimensionWidths_.size());

  // create the routers
  routerIterator.reset();
  routers_.setSize(dimensionWidths_);
  while (routerIterator.next(&routerAddress)) {
    std::string routerName = "Router_" +
      strop::vecString<u32>(routerAddress, '-');

    // use the router factory to create a router
    routers_.at(routerAddress) = RouterFactory::createRouter(
        routerName, this, routerAddress, routingAlgorithmFactory,
        _settings["router"]);
  }
  delete routingAlgorithmFactory;

  // link routers via channels
  routerIterator.reset();
  while (routerIterator.next(&routerAddress)) {
    u32 portBase = concentration_;
    for (u32 dim = 0; dim < dimensions_; dim++) {
      u32 dimWidth = dimensionWidths_.at(dim);
      u32 dimWeight = dimensionWeights_.at(dim);
      dbgprintf("dim=%u width=%u weight=%u\n", dim, dimWidth, dimWeight);

      for (u32 offset = 1; offset < dimWidth; offset++) {
        // determine the source router
        std::vector<u32> sourceAddress(routerAddress);

        // determine the destination router
        std::vector<u32> destinationAddress(sourceAddress);
        destinationAddress.at(dim) = (sourceAddress.at(dim) + offset) %
            dimWidth;

        for (u32 weight = 0; weight < dimWeight; weight++) {
          // create the channel
          std::string channelName = "Channel_" +
              strop::vecString<u32>(routerAddress, '-') + "-to-" +
              strop::vecString<u32>(destinationAddress, '-') +
              "-" + std::to_string(weight);
          Channel* channel = new Channel(channelName, this,
                                         _settings["internal_channel"]);
          internalChannels_.push_back(channel);

          // determine the port numbers
          u32 sourcePort = portBase + ((offset - 1) * dimWeight) + weight;
          u32 destinationPort = portBase + ((dimWidth - 1) * dimWeight) -
              (offset * dimWeight) + weight;
          dbgprintf("linking %s:%u to %s:%u with %s",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    sourcePort,
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    destinationPort,
                    channelName.c_str());

          // link the routers from source to destination
          routers_.at(sourceAddress)->setOutputChannel(sourcePort, channel);
          routers_.at(destinationAddress)->setInputChannel(destinationPort,
                                                           channel);
        }
      }
      portBase += ((dimWidth - 1) * dimWeight);
    }
  }

  // create a vector of dimension widths that contains the concentration
  std::vector<u32> fullDimensionWidths(1);
  fullDimensionWidths.at(0) = concentration_;
  fullDimensionWidths.insert(fullDimensionWidths.begin() + 1,
                             dimensionWidths_.begin(), dimensionWidths_.end());

  // create a injection algorithm factory
  InjectionAlgorithmFactory* injectionAlgorithmFactory =
      new InjectionAlgorithmFactory(numVcs_, _settings["routing"]);

  // create interfaces and link them with the routers
  interfaces_.setSize(fullDimensionWidths);
  u32 interfaceId = 0;
  routerIterator.reset();
  while (routerIterator.next(&routerAddress)) {
    // get the router now, for later linking with terminals
    Router* router = routers_.at(routerAddress);

    // loop over concentration
    for (u32 conc = 0; conc < concentration_; conc++) {
      // create a vector for the Interface address
      std::vector<u32> interfaceAddress(1);
      interfaceAddress.at(0) = conc;
      interfaceAddress.insert(interfaceAddress.begin() + 1,
                              routerAddress.begin(), routerAddress.end());

      // create an interface name
      std::string interfaceName = "Interface_" +
          strop::vecString<u32>(interfaceAddress, '-');

      // create the interface
      Interface* interface = InterfaceFactory::createInterface(
          interfaceName, this, interfaceId, injectionAlgorithmFactory,
          _settings["interface"]);
      interfaces_.at(interfaceAddress) = interface;
      interfaceId++;

      // create I/O channels
      std::string inChannelName = "Channel_" +
          strop::vecString<u32>(interfaceAddress, '-') + "-to-" +
          strop::vecString<u32>(routerAddress, '-');
      std::string outChannelName = "Channel_" +
          strop::vecString<u32>(routerAddress, '-') + "-to-" +
          strop::vecString<u32>(interfaceAddress, '-');
      Channel* inChannel = new Channel(inChannelName, this,
                                       _settings["external_channel"]);
      Channel* outChannel = new Channel(outChannelName, this,
                                        _settings["external_channel"]);
      externalChannels_.push_back(inChannel);
      externalChannels_.push_back(outChannel);

      // link with router
      router->setInputChannel(conc, inChannel);
      interface->setOutputChannel(inChannel);
      router->setOutputChannel(conc, outChannel);
      interface->setInputChannel(outChannel);
    }
  }

  delete injectionAlgorithmFactory;
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
  for (auto it = internalChannels_.begin();
       it != internalChannels_.end(); ++it) {
    delete *it;
  }
  for (auto it = externalChannels_.begin();
       it != externalChannels_.end(); ++it) {
    delete *it;
  }
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

void Network::translateIdToAddress(u32 _id, std::vector<u32>* _address) const {
  _address->resize(dimensions_ + 1);
  // addresses are in little endian format
  u32 mod, div;
  mod = _id % concentration_;
  div = _id / concentration_;
  _address->at(0) = mod;
  for (u32 dim = 0; dim < dimensions_; dim++) {
    u32 dimWidth = dimensionWidths_.at(dim);
    mod = div % dimWidth;
    div = div / dimWidth;
    _address->at(dim + 1) = mod;
  }
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin();
       it != externalChannels_.end(); ++it) {
    _channels->push_back(*it);
  }
  for (auto it = internalChannels_.begin();
       it != internalChannels_.end(); ++it) {
    _channels->push_back(*it);
  }
}

}  // namespace SlimFly
