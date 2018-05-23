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
#include "network/dragonfly/Network.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/cube/util.h"
#include "network/dragonfly/util.h"
#include "network/dragonfly/RoutingAlgorithm.h"
#include "util/DimensionIterator.h"

namespace Dragonfly {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // concentration
  assert(_settings.isMember("concentration"));
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);

  // local
  assert(_settings.isMember("local_width"));
  localWidth_ = _settings["local_width"].asUInt();
  assert(_settings.isMember("local_weight"));
  localWeight_ = _settings["local_weight"].asUInt();
  assert(localWidth_ > 0);
  assert(localWeight_ > 0);

  // global
  assert(_settings.isMember("global_width"));
  globalWidth_ = _settings["global_width"].asUInt();
  assert(_settings.isMember("global_weight"));
  globalWeight_ = _settings["global_weight"].asUInt();
  assert(globalWidth_ > 0);
  assert(globalWeight_ > 0);

  // channels
  assert(_settings.isMember("channel_mode"));
  assert(_settings.isMember("global_channel"));
  assert(_settings.isMember("local_channel"));
  assert(_settings.isMember("external_channel"));

  // scalars
  f64 global_scalar = 0.0;
  f64 local_scalar = 0.0;
  if (_settings["channel_mode"].asString() == "scalar") {
    assert(_settings.isMember("global_scalar"));
    assert(_settings.isMember("local_scalar"));
    global_scalar = _settings["global_scalar"].asFloat();
    local_scalar = _settings["local_scalar"].asFloat();
  }

  // radix
  groupRadix_ = ((globalWidth_ - 1) * globalWeight_);
  globalPortsPerRouter_ = (u32)std::ceil(groupRadix_ / (f64)localWidth_);
  routerGlobalPortBase_ = concentration_ + ((localWidth_ - 1) * localWeight_);
  routerRadix_ = concentration_ + ((localWidth_ - 1) * localWeight_) +
                 globalPortsPerRouter_;

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create routers
  routers_.resize(globalWidth_);
  for (u32 group = 0; group < globalWidth_; group++) {
    routers_.at(group).resize(localWidth_, nullptr);
    for (u32 r = 0; r < localWidth_; r++) {
      // router info
      std::vector<u32> routerAddress = {r, group};
      u32 routerId = translateRouterAddressToId(&routerAddress);
      std::vector<u32> tmpAdd;
      translateRouterIdToAddress(routerId, &tmpAdd);

      std::string rname = "Router_" + strop::vecString<u32>(routerAddress, '-');
      // make router
      routers_.at(group).at(r) = Router::create(
          rname, this, this, routerId, routerAddress, routerRadix_,
          numVcs_, protocolClassVcs_, _metadataHandler, _settings["router"]);
    }
  }

  // create global channels, link groups via global channels
  for (u32 srcGroup = 0; srcGroup < globalWidth_; srcGroup++) {
    for (u32 fwdOffset = 1; fwdOffset < globalWidth_; fwdOffset++) {
      u32 dstGroup = (srcGroup + fwdOffset) % globalWidth_;
      for (u32 weight = 0; weight < globalWeight_; weight++) {
        u32 reverseOffset = globalWidth_ - fwdOffset;
        u32 srcGroupPort, srcRouter, srcRouterPort;
        computeGlobalToRouterMap(weight, fwdOffset, &srcGroupPort,
                                 &srcRouter, &srcRouterPort);
        u32 dstGroupPort, dstRouter, dstRouterPort;
        computeGlobalToRouterMap(weight, reverseOffset, &dstGroupPort,
                                 &dstRouter, &dstRouterPort);
        std::vector<u32> srcAddress = {srcRouter, srcGroup};
        std::vector<u32> dstAddress = {dstRouter, dstGroup};

        // create channels
        std::string globalChannelName =
            "GlobalChannel_" +
            strop::vecString<u32>(srcAddress, '-') + "-to-" +
            strop::vecString<u32>(dstAddress, '-');

        // determine the global channel latency for current src dst group
        if (_settings["channel_mode"].asString() == "scalar") {
          f64 link_dist = fabs((s64)srcGroup - (s64)dstGroup);
          u32 channelLatency = (u32)(ceil(global_scalar * link_dist));

          // override settings
          _settings["global_channel"]["latency"] = channelLatency;
        }

        Channel* globalChannel = new Channel(
            globalChannelName, this, numVcs_, _settings["global_channel"]);
        globalChannels_.push_back(globalChannel);

        // link the routers from source to dst
        routers_.at(srcGroup).at(srcRouter)->setOutputChannel(srcRouterPort,
                                                              globalChannel);
        routers_.at(dstGroup).at(dstRouter)->setInputChannel(dstRouterPort,
                                                             globalChannel);
      }
    }
  }

  // create local channels, link routers via channels
  for (u32 group = 0; group < globalWidth_; group++) {
    for (u32 srcRouter = 0; srcRouter < localWidth_; srcRouter++) {
      u32 portBase = concentration_;
      for (u32 offset = 1; offset < localWidth_; offset++) {
        std::vector<u32> srcAddress = {srcRouter, group};

        u32 dstRouter = (srcRouter + offset) % localWidth_;
        std::vector<u32> dstAddress = {dstRouter, group};

        for (u32 weight = 0; weight < localWeight_; weight++) {
          // create the channel
          std::string channelName =
              "LocalChannel_" + strop::vecString<u32>(srcAddress, '-') +
              "-to-" + strop::vecString<u32>(dstAddress, '-') + "-" +
              std::to_string(weight);
          // determine the local channel latency
          if (_settings["channel_mode"].asString() == "scalar") {
            f64 link_dist = fabs((s64)srcRouter - (s64)dstRouter);
            u32 channelLatency = (u32)(ceil(local_scalar * link_dist));

            // override settings
            _settings["local_channel"]["latency"] = channelLatency;
          }

          Channel* channel = new Channel(channelName, this, numVcs_,
                                         _settings["local_channel"]);
          localChannels_.push_back(channel);

          // determine the port numbers
          u32 srcPort = computeLocalSrcPort(portBase, offset, weight);
          u32 dstPort = computeLocalDstPort(portBase, offset, weight);

          // link the routers from src to dst
          routers_.at(group).at(srcRouter)->setOutputChannel(srcPort,
                                                             channel);
          routers_.at(group).at(dstRouter)->setInputChannel(dstPort,
                                                            channel);
        }
      }
      portBase += ((localWidth_ - 1) * localWeight_);
    }
  }

  // create a vector of dimension widths that contains the concentration
  std::vector<u32> fullDimensionWidths = {concentration_,
                                          localWidth_, globalWidth_};

  // create interfaces and link them with the routers
  interfaces_.setSize(fullDimensionWidths);
  for (u32 group = 0; group < globalWidth_; group++) {
    for (u32 r = 0; r < localWidth_; r++) {
      // get the router now, for later linking with terminals
      Router* router = routers_.at(group).at(r);
      std::vector<u32> routerAddress({r, group});

      // loop over concentration
      for (u32 conc = 0; conc < concentration_; conc++) {
        // create a vector for the Interface address
        std::vector<u32> interfaceAddress({conc, r, group});

        // create an interface name
        std::string interfaceName =
            "Interface_" + strop::vecString<u32>(interfaceAddress, '-');

        // create the interface
        u32 interfaceId = translateInterfaceAddressToId(&interfaceAddress);
        std::vector<u32> tmpAdd;
        translateInterfaceIdToAddress(interfaceId, &tmpAdd);
        Interface* interface = Interface::create(
            interfaceName, this, interfaceId, interfaceAddress, numVcs_,
            protocolClassVcs_, _metadataHandler, _settings["interface"]);
        interfaces_.at(interfaceAddress) = interface;

        // create I/O channels
        std::string inChannelName =
            "Channel_" + strop::vecString<u32>(interfaceAddress, '-') + "-to-" +
            strop::vecString<u32>(routerAddress, '-');
        std::string outChannelName =
            "Channel_" + strop::vecString<u32>(routerAddress, '-') + "-to-" +
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
  }

  // only works if all ports are populated
  if ((((globalWidth_ -1) * globalWeight_) % localWidth_) == 0) {
    for (u32 g = 0; g < globalWidth_; g++) {
      for (u32 r = 0; r < localWidth_; r++) {
        Router* router = routers_.at(g).at(r);
        for (u32 p = 0; p < router->numPorts()-1; p++) {
          Channel* in = router->getInputChannel(p);
          Channel* out = router->getOutputChannel(p);
          assert(in != nullptr);
          assert(out != nullptr);
        }
      }
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();
}

Network::~Network() {
  // delete routers
  for (u32 g = 0; g < globalWidth_; g++) {
    for (u32 r = 0; r < localWidth_; r++) {
      delete routers_.at(g).at(r);
    }
  }

  // delete interfaces
  for (u32 id = 0; id < interfaces_.size(); id++) {
    delete interfaces_.at(id);
  }

  // delete channels
  for (auto it = globalChannels_.begin();
       it != globalChannels_.end(); ++it) {
    delete *it;
  }
  for (auto it = localChannels_.begin();
       it != localChannels_.end(); ++it) {
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
      localWidth_, localWeight_, globalWidth_, globalWeight_, concentration_,
      routerRadix_, globalPortsPerRouter_, info.settings);
}

u32 Network::numRouters() const {
  return localWidth_ * globalWidth_;
}

u32 Network::numInterfaces() const {
  return localWidth_ * concentration_ * globalWidth_;
}

Router* Network::getRouter(u32 _id) const {
  std::vector<u32> routerAddress;
  translateRouterIdToAddress(_id, &routerAddress);
  u32 g = routerAddress.at(0);
  u32 r = routerAddress.at(1);
  return routers_.at(g).at(r);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Dragonfly::translateInterfaceIdToAddress(
      concentration_, localWidth_, _id, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return Dragonfly::translateInterfaceAddressToId(
      concentration_, localWidth_, _address);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  Dragonfly::translateRouterIdToAddress(
      localWidth_, _id, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return Dragonfly::translateRouterAddressToId(
      localWidth_, _address);
}

u32 Network::computeMinimalHops(const std::vector<u32>* _source,
                                const std::vector<u32>* _destination) const {
  return Dragonfly::computeMinimalHops(_source, _destination,
                                       globalWidth_, globalWeight_,
                                       routerGlobalPortBase_,
                                       globalPortsPerRouter_,
                                       localWidth_);
}

void Network::computeGlobalToRouterMap(u32 _thisGlobalWeight,
                                       u32 _thisGlobalOffset,
                                       u32* _globalPort,
                                       u32* _localRouter, u32* _localPort) {
  Dragonfly::computeGlobalToRouterMap(routerGlobalPortBase_,
                                      globalPortsPerRouter_,
                                      globalWidth_,
                                      globalWeight_,
                                      localWidth_, _thisGlobalWeight,
                                      _thisGlobalOffset, _globalPort,
                                      _localRouter, _localPort);
}

u32 Network::computeLocalSrcPort(u32 _portBase, u32 _offset, u32 _weight) {
  return Dragonfly::computeLocalSrcPort(_portBase, _offset, localWeight_,
                                        _weight);
}

u32 Network::computeLocalDstPort(u32 _portBase, u32 _offset, u32 _weight) {
  return Dragonfly::computeLocalDstPort(_portBase, _offset, localWidth_,
                                        localWeight_, _weight);
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
  for (auto it = localChannels_.begin(); it != localChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
}

}  // namespace Dragonfly

registerWithObjectFactory("dragonfly", ::Network,
                          Dragonfly::Network, NETWORK_ARGS);
