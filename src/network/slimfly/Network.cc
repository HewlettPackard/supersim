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
#include <set>

#include "interface/InterfaceFactory.h"
#include "network/slimfly/InjectionAlgorithmFactory.h"
#include "network/slimfly/RoutingAlgorithmFactory.h"
#include "router/RouterFactory.h"
#include "util/DimensionIterator.h"
#include "network/slimfly/util.h"

namespace SlimFly {

Network::Network(const std::string& _name, const Component* _parent,
                 Json::Value _settings)
    : ::Network(_name, _parent, _settings) {
  // dimensions and concentration
  width_ = _settings["width"].asUInt();
  assert(isPrimePower(width_));
  concentration_ = _settings["concentration"].asUInt();
  assert(concentration_ > 0);
  dbgprintf("width_ = %u", width_);
  dbgprintf("concentration_ = %u", concentration_);

  // router radix
  assert(_settings["router"].isMember("num_ports") == false);
  u32 coeff = round(width_ / 4);
  int delta = width_ - 4*coeff;
  u32 networkRadix = (3 * width_ - delta) / 2;
  u32 routerRadix = concentration_ + networkRadix;
  std::vector<u32> dimensionWidths_ = {2, width_, width_};

  // create generator sets
  createGeneratorSet(width_, delta, X_, X_i_);

  _settings["router"]["num_ports"] = Json::Value(routerRadix);
  _settings["router"]["num_vcs"] = Json::Value(numVcs_);
  _settings["interface"]["num_vcs"] = Json::Value(numVcs_);

  routingTables_.setSize(dimensionWidths_);

  // create a routing algorithm factory to give to the routers
  RoutingAlgorithmFactory* routingAlgorithmFactory =
      new SlimFly::RoutingAlgorithmFactory(
          numVcs_, dimensionWidths_, concentration_, _settings["routing"],
          routingTables_, X_, X_i_);

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
    routingTables_.at(routerAddress) = new RoutingTable(routerAddress);
  }
  delete routingAlgorithmFactory;

  static const u32 NUM_GRAPHS = dimensionWidths_.at(0);
  // link routers via channels: Intra subgraph connections
  for (u32 graph = 0; graph < NUM_GRAPHS; graph++) {
    std::vector<u32>& dVtr = (graph == 0) ? X_ : X_i_;  // Deep copy
    std::set<u32> distSet(dVtr.begin(), dVtr.end());
    for (u32 col = 0; col < width_; col++) {
      std::vector<u32> inPorts(width_, 0), outPorts(width_, 0);
      for (u32 srcRow = 0; srcRow < width_; srcRow++) {
        for (u32 dstRow = 0; dstRow < width_; dstRow++) {
          // determine the source and destination router
          std::vector<u32> srcAddr = {graph, col, srcRow};
          std::vector<u32> dstAddr = {graph, col, dstRow};
          u32 dist = static_cast<u32>(
            std::abs<int>(static_cast<int>(dstRow) - srcRow));

          if (distSet.count(dist)) {
            // create the channel
            std::string chanName = "Channel_" +
              strop::vecString<u32>(srcAddr, '-') + "-to-" +
              strop::vecString<u32>(dstAddr, '-');
            Channel* channel = new Channel(
                chanName, this, _settings["internal_channel"]);
            internalChannels_.push_back(channel);

            // determine the port numbers
            u32 outPort = concentration_ + outPorts.at(srcRow)++;
            u32 inPort = concentration_ + inPorts.at(dstRow)++;

            // link the routers from source to destination
            routers_.at(srcAddr)->setOutputChannel(outPort, channel);
            routers_.at(dstAddr)->setInputChannel(inPort, channel);
            routingTables_.at(srcAddr)->addHop(outPort, dstAddr);
          }
        }
      }
    }
  }

  std::vector< std::vector<u32> > inputPorts(
      NUM_GRAPHS*width_, std::vector<u32>(width_, concentration_ + X_.size()));
  std::vector< std::vector<u32> > outputPorts(
      NUM_GRAPHS*width_, std::vector<u32>(width_, concentration_ + X_.size()));

  // link routers via channels: Inter subgraph connections
  for (u32 x = 0; x < width_; x++) {
    for (u32 y = 0; y < width_; y++) {
      for (u32 m = 0; m < width_; m++) {
        for (u32 c = 0; c < width_; c++) {
          if (y == ((m*x + c) % width_)) {
             // determine the source router
            std::vector<u32> addr1 = {0, x, y};
            std::vector<u32> addr2 = {1, m, c};

            // create the channel
            std::string fwdChannelName = "Channel_" +
              strop::vecString<u32>(addr1, '-') + "-to-" +
              strop::vecString<u32>(addr2, '-');
            std::string revChannelName = "Channel_" +
              strop::vecString<u32>(addr2, '-') + "-to-" +
              strop::vecString<u32>(addr1, '-');
            Channel* fwdChannel = new Channel(fwdChannelName, this,
                                        _settings["internal_channel"]);
            Channel* revChannel = new Channel(revChannelName, this,
                                        _settings["internal_channel"]);
            internalChannels_.push_back(fwdChannel);
            internalChannels_.push_back(revChannel);

             // link the routers from source to destination
            u32 outPort0 = outputPorts.at(x).at(y)++;
            u32 inPort0 = inputPorts.at(x).at(y)++;
            u32 outPort1 = outputPorts.at(width_ + m).at(c)++;
            u32 inPort1 = inputPorts.at(width_ + m).at(c)++;

            routers_.at(addr1)->setOutputChannel(outPort0, fwdChannel);
            routers_.at(addr2)->setInputChannel(inPort1, fwdChannel);
            routingTables_.at(addr1)->addHop(outPort0, addr2);

            routers_.at(addr2)->setOutputChannel(outPort1, revChannel);
            routers_.at(addr1)->setInputChannel(inPort0, revChannel);
            routingTables_.at(addr2)->addHop(outPort1, addr1);
          }
        }
      }
    }
  }

  for (u32 src = 0; src < routingTables_.size(); src++) {
    for (u32 dst = 0; dst < routingTables_.size(); dst++) {
      const std::vector<u32>& dstAddr = routingTables_.at(dst)->getAddr();
      // Check if there is a pat from src -> dst
      // If so, move on
      if (routingTables_.at(src)->getNumHops(dstAddr)) continue;
      // If there isnt a path look for an intermediate
      // node that does. Because Slim Fly has a max
      // diameter of 2, this does not have to be recursive
      for (u32 hop = 0; hop < routingTables_.size(); hop++) {
        const std::vector<u32>& hopAddr = routingTables_.at(hop)->getAddr();
        if (routingTables_.at(src)->getNumHops(hopAddr) == 1 &&
            routingTables_.at(hop)->getNumHops(dstAddr) == 1) {
          routingTables_.at(src)->addPath(dstAddr, hopAddr);
        }
      }
    }
  }

  for (u32 src = 0; src < routingTables_.size(); src++) {
    for (u32 dst = 0; dst < routingTables_.size(); dst++) {
      const std::vector<u32>& dstAddr = routingTables_.at(dst)->getAddr();
      if (routingTables_.at(src)->getPaths(dstAddr).size() == 0) {
        printf("No path from %s -> %s\n",
          strop::vecString<u32>(routingTables_.at(src)->getAddr()).c_str(),
          strop::vecString<u32>(dstAddr).c_str());
        assert(false);
      }
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
    delete routingTables_.at(id);
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
  computeAddress(_id, width_, concentration_, _address);
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
