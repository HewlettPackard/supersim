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

#include <cassert>
#include <cmath>

#include "interface/InterfaceFactory.h"
#include "network/foldedclos/InjectionAlgorithmFactory.h"
#include "network/foldedclos/RoutingAlgorithmFactory.h"
#include "router/RouterFactory.h"

namespace FoldedClos {

Network::Network(const std::string& _name, const Component* _parent,
                 Json::Value _settings)
    : ::Network(_name, _parent, _settings) {
  assert(_settings["router"].isMember("num_ports") == true);
  routerRadix_ = _settings["router"]["num_ports"].asUInt();
  _settings["router"]["num_vcs"] = Json::Value(numVcs_);
  _settings["interface"]["num_vcs"] = Json::Value(numVcs_);
  assert(routerRadix_ % 2 == 0);
  halfRadix_   = routerRadix_ / 2;
  numLevels_   = _settings["levels"].asUInt();
  rowRouters_  = (u32)pow(halfRadix_, numLevels_-1);
  numPorts_    = (u32)pow(halfRadix_, numLevels_);

  // create all routers
  routers_.resize(numLevels_);
  for (u32 r = 0; r < numLevels_; r++) {
    u32 row = numLevels_ - r - 1;
    u32 rowGroupSize = (u32)pow(halfRadix_, row);
    routers_.at(row).resize(rowRouters_);

    for (u32 col = 0; col < rowRouters_; col++) {
      std::vector<u32> routerAddress(numLevels_, U32_MAX);
      if (row < (numLevels_ - 1)) {
        // copy in upper router
        const std::vector<u32>& upperRouterAddress =
            routers_.at(row + 1).at(col)->getAddress();
        auto itb = upperRouterAddress.cbegin();
        auto ite = upperRouterAddress.cend();
        routerAddress.assign(itb, ite);

        // set the value of this level
        // u32 dunno = col % rowGroupSize;
        routerAddress.at(row+1) = (col / rowGroupSize) % halfRadix_;
      }

      // router name
      std::string rname = "Router_" + std::to_string(row) + "-" +
          std::to_string(col) + "_[";
      for (u32 i = 0; i < routerAddress.size(); i++) {
        if (routerAddress.at(i) == U32_MAX) {
          rname +=  "*";
        } else {
          rname += std::to_string(routerAddress.at(i));
        }

        if (i < routerAddress.size() - 1) {
          rname += ',';
        }
      }
      rname += ']';

      // create a routing algorithm factory
      RoutingAlgorithmFactory* routingAlgorithmFactory =
          new RoutingAlgorithmFactory(numVcs_, routerRadix_, numLevels_, row,
                                      _settings["routing"]);

      // make router
      routers_.at(row).at(col) = RouterFactory::createRouter(
          rname, this, routerAddress, routingAlgorithmFactory,
          _settings["router"]);

      // delete the routing algorithm factory
      delete routingAlgorithmFactory;
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

        Router* thisRouter = routers_.at(r).at(c);
        Router* thatRouter = routers_.at(thatRow).at(thatColumn);

        thisRouter->setInputChannel(thisPort, down);
        thisRouter->setOutputChannel(thisPort, up);
        thatRouter->setInputChannel(thatPort, up);
        thatRouter->setOutputChannel(thatPort, down);
      }
    }
  }

  InjectionAlgorithmFactory* injectionAlgorithmFactory =
      new InjectionAlgorithmFactory(numVcs_, _settings["routing"]);

  // create interfaces, external channels, link together
  u32 interfaceId = 0;
  interfaces_.resize(rowRouters_);
  for (u32 c = 0; c < rowRouters_; c++) {
    interfaces_.at(c).resize(halfRadix_);
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
      Router* router = routers_.at(r).at(c);
      router->setInputChannel(p, inChannel);
      router->setOutputChannel(p, outChannel);

      // create interface
      std::string interfaceName = "Interface_" + std::to_string(c) + ":" +
          std::to_string(p);
      Interface* interface = InterfaceFactory::createInterface(
          interfaceName, this, interfaceId, injectionAlgorithmFactory,
          _settings["interface"]);
      interfaces_.at(c).at(p) = interface;
      interfaceId++;

      // link to interface
      interface->setInputChannel(outChannel);
      interface->setOutputChannel(inChannel);
    }
  }

  delete injectionAlgorithmFactory;

  for (u32 id = 0; id < numInterfaces(); id++) {
    assert(getInterface(id) != nullptr);
  }
  for (u32 id = 0; id < numRouters(); id++) {
    assert(getRouter(id) != nullptr);
  }
}

Network::~Network() {
  // delete routers
  for (u32 r = 0; r < numLevels_; r++) {
    for (u32 c = 0; c < rowRouters_; c++) {
      delete routers_.at(r).at(c);
    }
  }
  // delete interfaces
  for (u32 c = 0; c < rowRouters_; c++) {
    for (u32 p = 0; p < halfRadix_; p++) {
      delete interfaces_.at(c).at(p);
    }
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
  u32 row = _id / rowRouters_;
  u32 col = _id % rowRouters_;
  return routers_.at(row).at(col);
}

Interface* Network::getInterface(u32 _id) const {
  u32 col = _id / halfRadix_;
  u32 port = _id % halfRadix_;
  return interfaces_.at(col).at(port);
}

void Network::translateIdToAddress(u32 _id, std::vector<u32>* _address) const {
  _address->resize(numLevels_);
  // work in reverse for little endian format
  for (u32 exp = 0, row = numLevels_ - 1; exp < numLevels_; exp++, row--) {
    u32 divisor = (u32)pow(halfRadix_, row);
    _address->at(row) = _id / divisor;
    _id %= divisor;
  }
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
