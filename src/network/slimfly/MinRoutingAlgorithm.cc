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
#include "network/slimfly/MinRoutingAlgorithm.h"

#include <cassert>

#include <unordered_set>
#include <set>

#include "types/Message.h"
#include "types/Packet.h"

namespace SlimFly {

MinRoutingAlgorithm::MinRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration,
    const std::vector<u32>& _X, const std::vector<u32>& _X_i)
    : RoutingAlgorithm(_name, _parent, _router, _latency),
      numVcs_(router_->numVcs()), dimensionWidths_(_dimensionWidths),
      concentration_(_concentration), X_(_X), X_i_(_X_i) {}

MinRoutingAlgorithm::~MinRoutingAlgorithm() {}

bool MinRoutingAlgorithm::checkConnected(
    u32 graph, u32 srcRow, u32 dstRow) {
  const std::vector<u32>& dVtr = (graph == 0) ? X_ : X_i_;
  std::set<u32> distSet(dVtr.begin(), dVtr.end());
  u32 dist = static_cast<u32>(
      std::abs<int>(static_cast<int>(dstRow) - srcRow));
  if (distSet.count(dist) != 0) return true;
  return false;
}

bool MinRoutingAlgorithm::checkConnectedAcross(
    const std::vector<u32>& routerAddress,
    const std::vector<u32>* destinationAddress) {
  u32 x1, y1, x2, y2;
  x1 = destinationAddress->at(2);
  y1 = destinationAddress->at(3);
  x2 = routerAddress.at(1);
  y2 = routerAddress.at(2);
  if (routerAddress.at(0)) {     // 1->0
    return (y1 == (x2*x1+y2)%dimensionWidths_.at(1));
  }
  // 0->1
  return (y2 == (x1*x2+y1)%dimensionWidths_.at(1));
}

void MinRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  std::unordered_set<u32> outputPorts;

  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->getAddress();
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress =
      _flit->getPacket()->getMessage()->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));

  std::vector<u32> nextHop;
  for (u32 i = 1; i < destinationAddress->size(); i++) {
    nextHop.push_back(destinationAddress->at(i));
  }
  u32 width = dimensionWidths_.at(1);
  RoutingTable rt(routerAddress);

  u32 dim;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != destinationAddress->at(dim+1)) {
      break;
    }
  }

  // test if already at destination router
  if (dim == routerAddress.size()) {
    bool res = outputPorts.insert(destinationAddress->at(0)).second;
    (void)res;
    assert(res);
  } else if (dim == 2) {   // same column
    u32 graph = routerAddress.at(0);
    u32 srcRow = routerAddress.at(dim);
    u32 dstRow = destinationAddress->at(dim + 1);
    if (checkConnected(graph, srcRow, dstRow)) {
      bool res = outputPorts.insert(rt.getPortNum(nextHop)).second;
      (void)res;
      assert(res);
    } else {
      for (u32 intRow = 0; intRow < width; intRow++) {
        nextHop.at(dim) = intRow;
        bool srcToInt = checkConnected(graph, srcRow, intRow);
        bool intToDst = checkConnected(graph, intRow, dstRow);
        if (srcToInt && intToDst) {
          bool res = outputPorts.insert(rt.getPortNum(nextHop)).second;
          (void)res;
          assert(res);
        }
      }
    }
  } else if (dim == 1) {   // same subgraph, diff col
    nextHop.at(0) = (routerAddress.at(0)) ? 0 : 1;
    for (u32 intCol = 0; intCol < width; intCol++) {
      for (u32 intRow = 0; intRow < width; intRow++) {
        nextHop.at(1) = intCol;
        nextHop.at(2) = intRow;
        std::vector<u32> next(nextHop);
        next.insert(next.begin(), 0);   // [dummy, nextHop]
        bool srcToInt = checkConnectedAcross(routerAddress, &next);
        bool intToDst = checkConnectedAcross(nextHop, destinationAddress);
        if (srcToInt && intToDst) {
          bool res = outputPorts.insert(rt.getPortNum(nextHop)).second;
          (void)res;
          assert(res);
        }
      }
    }
  } else {   // different subgraph
    if (checkConnectedAcross(routerAddress, destinationAddress)) {
      bool res = outputPorts.insert(rt.getPortNum(nextHop)).second;
      (void)res;
      assert(res);
    } else {
      nextHop.at(0) = routerAddress.at(0);
      nextHop.at(1) = routerAddress.at(1);
      for (u32 intRow = 0; intRow < width; intRow++) {
        nextHop.at(2) = intRow;
        bool srcToInt = checkConnected(
            routerAddress.at(0), routerAddress.at(2), intRow);
        bool intToDst = checkConnectedAcross(
            nextHop, destinationAddress);
        if (srcToInt && intToDst) {
          bool res = outputPorts.insert(rt.getPortNum(nextHop)).second;
          (void)res;
          assert(res);
        }
      }
    }
  }

  assert(outputPorts.size() > 0);
  for (auto it = outputPorts.cbegin(); it != outputPorts.cend(); ++it) {
    u32 outputPort = *it;
    // select all VCs in the output port
    for (u32 vc = 0; vc < numVcs_; vc++) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace SlimFly
