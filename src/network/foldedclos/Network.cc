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
#include "network/foldedclos/Network.h"

#include <factory/Factory.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include <tuple>

#include "network/foldedclos/RoutingAlgorithm.h"
#include "network/foldedclos/util.h"

namespace FoldedClos {

Network::Network(const std::string& _name, const Component* _parent,
                 MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Network(_name, _parent, _metadataHandler, _settings) {
  // network structure and router radix
  routerRadix_ = _settings["radix"].asUInt();
  assert(routerRadix_ >= 2);
  assert(routerRadix_ % 2 == 0);
  halfRadix_   = routerRadix_ / 2;
  numLevels_   = _settings["levels"].asUInt();
  assert(numLevels_ >= 1);
  rowRouters_  = (u32)pow(halfRadix_, numLevels_-1);

  // parse the protocol classes description
  loadProtocolClassInfo(_settings["protocol_classes"]);

  // create all routers
  routers_.resize(numLevels_ * rowRouters_, nullptr);
  for (u32 row = 0; row < numLevels_; row++) {
    for (u32 col = 0; col < rowRouters_; col++) {
      // router info
      u32 routerId = row * rowRouters_ + col;
      std::vector<u32> routerAddress;
      translateRouterIdToAddress(routerId, &routerAddress);
      std::string rname = "Router_" + strop::vecString<u32>(routerAddress, '-');

      // make router
      routers_.at(routerId) = Router::create(
          rname, this, this, routerId, routerAddress, routerRadix_, numVcs_,
          _metadataHandler, _settings["router"]);
    }
  }

  // create internal channels, link routers via channels
  for (u32 r = 0; r < numLevels_ - 1; r++) {
    for (u32 c = 0; c < rowRouters_; c++) {
      for (u32 p = 0; p < halfRadix_; p++) {
        // create channels
        std::string upChannelName = "UpChannel_" + std::to_string(r) +
            ":" + std::to_string(c) + ":" + std::to_string(p);
        Channel* up = new Channel(upChannelName, this, numVcs_,
                                  _settings["internal_channel"]);
        internalChannels_.push_back(up);
        std::string downChannelName = "DownChannel_" + std::to_string(r) +
            ":" + std::to_string(c) + ":" + std::to_string(p);
        Channel* down = new Channel(downChannelName, this, numVcs_,
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

  // create interfaces, external channels, link together
  interfaces_.resize(rowRouters_ * halfRadix_, nullptr);
  for (u32 c = 0; c < rowRouters_; c++) {
    for (u32 p = 0; p < halfRadix_; p++) {
      u32 r = 0;

      // create channels
      std::string inChannelName = "InChannel_" + std::to_string(r) +
          ":" + std::to_string(c) + ":" + std::to_string(p);
      Channel* inChannel = new Channel(inChannelName, this, numVcs_,
                                       _settings["external_channel"]);
      externalChannels_.push_back(inChannel);
      std::string outChannelName = "OutChannel_" + std::to_string(r) +
          ":" + std::to_string(c) + ":" + std::to_string(p);
      Channel* outChannel = new Channel(outChannelName, this, numVcs_,
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
      Interface* interface = Interface::create(
          interfaceName, this, interfaceId, interfaceAddress, numVcs_,
          protocolClassVcs_, _metadataHandler, _settings["interface"]);
      interfaces_.at(interfaceId) = interface;

      // link to interface
      interface->setInputChannel(0, outChannel);
      interface->setOutputChannel(0, inChannel);
    }
  }

  // clear the protocol class info
  clearProtocolClassInfo();

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

::RoutingAlgorithm* Network::createRoutingAlgorithm(
     u32 _inputPort, u32 _inputVc, const std::string& _name,
     const Component* _parent, Router* _router) {
  // get the info
  const Network::RoutingAlgorithmInfo& info =
      routingAlgorithmInfo_.at(_inputVc);

  // call the routing algorithm factory
  return RoutingAlgorithm::create(
      _name, _parent, _router, info.baseVc, info.numVcs, _inputPort, _inputVc,
      routerRadix_, numLevels_, info.settings);
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

registerWithFactory("folded_clos", ::Network,
                    FoldedClos::Network, NETWORK_ARGS);
