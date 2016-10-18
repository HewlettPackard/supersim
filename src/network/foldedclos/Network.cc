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
#include "network/foldedclos/Network.h"

#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "interface/InterfaceFactory.h"
#include "network/foldedclos/RoutingAlgorithmFactory.h"
#include "network/foldedclos/util.h"
#include "router/RouterFactory.h"

namespace FoldedClos {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // network structure and router radix
  u32 routerRadix = _settings["radix"].asUInt();
  assert(routerRadix >= 2);
  assert(routerRadix % 2 == 0);
  halfRadix_   = routerRadix / 2;
  numLevels_   = _settings["levels"].asUInt();
  assert(numLevels_ >= 1);
  rowRouters_  = (u32)pow(halfRadix_, numLevels_-1);

  // create all routers
  routers_.resize(numLevels_ * rowRouters_, nullptr);
  for (u32 row = 0; row < numLevels_; row++) {
    for (u32 col = 0; col < rowRouters_; col++) {
      // router info
      u32 routerId = row * rowRouters_ + col;
      std::vector<u32> routerAddress;
      translateRouterIdToAddress(routerId, &routerAddress);
      std::string rname = "Router_" + strop::vecString<u32>(routerAddress, '-');

      // parse the traffic classes description
      std::vector<::RoutingAlgorithmFactory*> routingAlgorithmFactories;
      for (u32 idx = 0; idx < _settings["traffic_classes"].size(); idx++) {
        u32 numVcs = _settings["traffic_classes"][idx]["num_vcs"].asUInt();
        u32 baseVc = routingAlgorithmFactories.size();
        for (u32 vc = 0; vc < numVcs; vc++) {
          routingAlgorithmFactories.push_back(
              new RoutingAlgorithmFactory(
                  baseVc, numVcs, routerRadix, numLevels_,
                  _settings["traffic_classes"][idx]["routing"]));
        }
      }

      // make router
      routers_.at(routerId) = RouterFactory::createRouter(
          rname, this, routerId, routerAddress, routerRadix, numVcs_,
          _metadataHandler, &routingAlgorithmFactories, _settings["router"]);

      // we don't need the routing algorithm factories anymore
      for (::RoutingAlgorithmFactory* raf : routingAlgorithmFactories) {
        delete raf;
      }
      routingAlgorithmFactories.clear();
    }
  }

  // create internal channels, link routers via channels
  for (u32 r = 0; r < numLevels_ - 1; r++) {
    for (u32 c = 0; c < rowRouters_; c++) {
      for (u32 p = 0; p < halfRadix_; p++) {
        // create channels
        std::string upChannelName = "UpChannel_" + std::to_string(r) +
            ":" + std::to_string(c) + ":" + std::to_string(p);
        Channel* up = new Channel(upChannelName, this,
                                  _settings["internal_channel"]);
        internalChannels_.push_back(up);
        std::string downChannelName = "DownChannel_" + std::to_string(r) +
            ":" + std::to_string(c) + ":" + std::to_string(p);
        Channel* down = new Channel(downChannelName, this,
                                    _settings["internal_channel"]);
        internalChannels_.push_back(down);

        // compute src and dst
        u32 thisGroupSize = (u32)pow(halfRadix_, r);
        u32 thisGroup = c / thisGroupSize;
        u32 thisBase = thisGroup * thisGroupSize;
        u32 thisOffset = c - thisBase;
        assert(thisOffset == (c % thisGroupSize));

        u32 thatGroupSize = (u32)pow(halfRadix_, r+1);
        u32 thatGroup = c / thatGroupSize;
        u32 thatBase = thatGroup * thatGroupSize;
        u32 thatOffset = c - thatBase;
        (void)thatOffset;  // unused
        assert(thatOffset == (c % thatGroupSize));

        u32 thisRow    = r;
        u32 thisColumn = c;
        u32 thisPort = halfRadix_ + p;

        u32 thatRow = r + 1;
        u32 thatColumn = thatBase + thisOffset + (p * thisGroupSize);
        u32 thatPort = thisGroup % halfRadix_;

        dbgprintf("[R,C,P]: [%u,%u,%u] -> [%u,%u,%u]\n",
                  thisRow, thisColumn, thisPort,
                  thatRow, thatColumn, thatPort);

        u32 thisId = thisRow * rowRouters_ + thisColumn;
        u32 thatId = thatRow * rowRouters_ + thatColumn;
        Router* thisRouter = routers_.at(thisId);
        Router* thatRouter = routers_.at(thatId);

        thisRouter->setInputChannel(thisPort, down);
        thisRouter->setOutputChannel(thisPort, up);
        thatRouter->setInputChannel(thatPort, up);
        thatRouter->setOutputChannel(thatPort, down);
      }
    }
  }

  // parse the traffic classes description
  std::vector<std::tuple<u32, u32> > trafficClassVcs;
  for (u32 baseVc = 0, idx = 0; idx < _settings["traffic_classes"].size();
       idx++) {
    u32 numVcs = _settings["traffic_classes"][idx]["num_vcs"].asUInt();
    trafficClassVcs.push_back(std::make_tuple(baseVc, numVcs));
    baseVc += numVcs;
  }

  // create interfaces, external channels, link together
  interfaces_.resize(rowRouters_ * halfRadix_, nullptr);
  for (u32 c = 0; c < rowRouters_; c++) {
    for (u32 p = 0; p < halfRadix_; p++) {
      u32 r = 0;

      // create channels
      std::string inChannelName = "InChannel_" + std::to_string(r) +
          ":" + std::to_string(c) + ":" + std::to_string(p);
      Channel* inChannel = new Channel(inChannelName, this,
                                       _settings["external_channel"]);
      externalChannels_.push_back(inChannel);
      std::string outChannelName = "OutChannel_" + std::to_string(r) +
          ":" + std::to_string(c) + ":" + std::to_string(p);
      Channel* outChannel = new Channel(outChannelName, this,
                                        _settings["external_channel"]);
      externalChannels_.push_back(outChannel);

      // link to router
      u32 routerId = r * rowRouters_ + c;
      Router* router = routers_.at(routerId);
      router->setInputChannel(p, inChannel);
      router->setOutputChannel(p, outChannel);

      // interface id and address
      u32 interfaceId = c * halfRadix_ + p;
      std::vector<u32> interfaceAddress;
      translateInterfaceIdToAddress(interfaceId, &interfaceAddress);

      // create interface
      std::string interfaceName = "Interface_" + std::to_string(c) + ":" +
          std::to_string(p);
      Interface* interface = InterfaceFactory::createInterface(
          interfaceName, this, interfaceId, interfaceAddress, numVcs_,
          trafficClassVcs, _settings["interface"]);
      interfaces_.at(interfaceId) = interface;

      // link to interface
      interface->setInputChannel(0, outChannel);
      interface->setOutputChannel(0, inChannel);
    }
  }

  for (u32 id = 0; id < numInterfaces(); id++) {
    assert(getInterface(id) != nullptr);
  }
  for (u32 id = 0; id < numRouters(); id++) {
    assert(getRouter(id) != nullptr);
  }
}

Network::~Network() {
  // delete routers
  for (auto it = routers_.begin(); it != routers_.end(); ++it) {
    Router* router = *it;
    delete router;
  }
  // delete interfaces
  for (auto it = interfaces_.begin(); it != interfaces_.end(); ++it) {
    Interface* interface = *it;
    delete interface;
  }
  // delete channels
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* c = *it;
    delete c;
  }
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* c = *it;
    delete c;
  }
}

u32 Network::numRouters() const {
  return numLevels_ * rowRouters_;
}

u32 Network::numInterfaces() const {
  return rowRouters_ * halfRadix_;
}

Router* Network::getRouter(u32 _id) const {
  return routers_.at(_id);
}

Interface* Network::getInterface(u32 _id) const {
  return interfaces_.at(_id);
}

void Network::translateInterfaceIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  FoldedClos::translateInterfaceIdToAddress(halfRadix_, numLevels_, rowRouters_,
                                            _id, _address);
}

u32 Network::translateInterfaceAddressToId(
    const std::vector<u32>* _address) const {
  return FoldedClos::translateInterfaceAddressToId(halfRadix_, numLevels_,
                                                   rowRouters_, _address);
}

void Network::translateRouterIdToAddress(
    u32 _id, std::vector<u32>* _address) const {
  FoldedClos::translateRouterIdToAddress(halfRadix_, numLevels_, rowRouters_,
                                         _id, _address);
}

u32 Network::translateRouterAddressToId(
    const std::vector<u32>* _address) const {
  return FoldedClos::translateRouterAddressToId(halfRadix_, numLevels_,
                                                rowRouters_, _address);
}

void Network::collectChannels(std::vector<Channel*>* _channels) {
  for (auto it = externalChannels_.begin(); it != externalChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
  for (auto it = internalChannels_.begin(); it != internalChannels_.end();
       ++it) {
    Channel* c = *it;
    _channels->push_back(c);
  }
}

}  // namespace FoldedClos
