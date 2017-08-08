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
#include "network/hyperx/Network.h"

#include <factory/Factory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/cube/util.h"
#include "network/hyperx/RoutingAlgorithm.h"
#include "util/DimensionIterator.h"

namespace HyperX {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
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

  // varying channel latency per dimension
  std::vector<f64> scalars;
  assert(_settings.isMember("channel_mode"));

  // scalar
  if (_settings["channel_mode"].asString() == "scalar") {
    assert(_settings["channel_scalars"].isArray());
    assert(_settings["channel_scalars"].size() == dimensions_);
    scalars.resize(dimensions_);
    for (u32 i = 0; i < dimensions_; i++) {
      if ( _settings["channel_scalars"][i].asFloat() > 0.0 ) {
        scalars.at(i) = _settings["channel_scalars"][i].asFloat();
      } else {
        scalars.at(i) = 1.0;
      }
    }
    dbgprintf("scalars = %s",
              strop::vecString<f64>(scalars, ',').c_str());
  }

  // router radix
  u32 routerRadix = concentration_;
  for (u32 i = 0; i < dimensions_; i++) {
    routerRadix += ((dimensionWidths_.at(i) - 1) * dimensionWeights_.at(i));
  }

  // parse the traffic classes description
  loadTrafficClassInfo(_settings["traffic_classes"]);

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
    u32 routerId = translateRouterAddressToId(&routerAddress);
    routers_.at(routerAddress) = Router::create(
        routerName, this, this, routerId, routerAddress, routerRadix, numVcs_,
        _metadataHandler, _settings["router"]);
  }

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

        // determine the channel latency for current dim and offset
        if (_settings["channel_mode"].asString() == "scalar") {
          f64 link_dist = fabs((s64)sourceAddress.at(dim) -
                               (s64)destinationAddress.at(dim));
          u32 channelLatency = (u32)(ceil(scalars[dim] * link_dist));
          dbgprintf("s=%s d=%s c_dist=%.2f l_val=%.2f latency=%u",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    link_dist, scalars[dim], channelLatency);

          // override settings
          _settings["internal_channel"]["latency"] = channelLatency;
        }

        for (u32 weight = 0; weight < dimWeight; weight++) {
          // create the channel
          std::string channelName = "Channel_" +
              strop::vecString<u32>(routerAddress, '-') + "-to-" +
              strop::vecString<u32>(destinationAddress, '-') +
              "-" + std::to_string(weight);
          Channel* channel = new Channel(channelName, this, numVcs_,
                                         _settings["internal_channel"]);
          internalChannels_.push_back(channel);

          // determine the port numbers
          u32 sourcePort = portBase + ((offset - 1) * dimWeight) + weight;
          u32 destinationPort = portBase + ((dimWidth - 1) * dimWeight) -
              (offset * dimWeight) + weight;
          dbgprintf("s=%s:%u to d=%s:%u with %s latency=%d",
                    strop::vecString<u32>(sourceAddress, '-').c_str(),
                    sourcePort,
                    strop::vecString<u32>(destinationAddress, '-').c_str(),
                    destinationPort,
                    channelName.c_str(), channel->latency());

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

  // create interfaces and link them with the routers
  interfaces_.setSize(fullDimensionWidths);
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
      u32 interfaceId = translateInterfaceAddressToId(&interfaceAddress);
      Interface* interface = Interface::create(
          interfaceName, this, interfaceId, interfaceAddress, numVcs_,
          trafficClassVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceAddress) = interface;

      // create I/O channels
      std::string inChannelName = "Channel_" +
          strop::vecString<u32>(interfaceAddress, '-') + "-to-" +
          strop::vecString<u32>(routerAddress, '-');
      std::string outChannelName = "Channel_" +
          strop::vecString<u32>(routerAddress, '-') + "-to-" +
          strop::vecString<u32>(interfaceAddress, '-');
      Channel* inChannel = new Channel(inChannelName, this, numVcs_,
                                       _settings["external_channel"]);
      Channel* outChannel = new Channel(outChannelName, this, numVcs_,
                                        _settings["external_channel"]);
      externalChannels_.push_back(inChannel);
      externalChannels_.push_back(outChannel);

      // link with router
      router->setInputChannel(conc, inChannel);
      interface->setOutputChannel(0, inChannel);
      router->setOutputChannel(conc, outChannel);
      interface->setInputChannel(0, outChannel);
    }
  }

  // clear the traffic class info
  clearTrafficClassInfo();
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

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _inputPort, u32 _inputVc, const std::string& _name,
     const Component* _parent, Router* _router) {
  // get the info
  const Network::RoutingAlgorithmInfo& info =
      routingAlgorithmInfo_.at(_inputVc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(
      _name, _parent, _router, info.baseVc, info.numVcs, _inputPort, _inputVc,
      dimensionWidths_, dimensionWeights_, concentration_, info.settings);
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
  Cube::computeTerminalAddress(_id, dimensionWidths_, concentration_, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return Cube::computeTerminalId(_address, dimensionWidths_, concentration_);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Cube::computeRouterAddress(_id, dimensionWidths_, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return Cube::computeRouterId(_address, dimensionWidths_);
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

}  // namespace HyperX

registerWithFactory("hyperx", ::Network,
                    HyperX::Network, NETWORK_ARGS);
