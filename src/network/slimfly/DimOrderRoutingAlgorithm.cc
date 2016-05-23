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
#include "network/slimfly/DimOrderRoutingAlgorithm.h"

#include <strop/strop.h>
#include <cassert>
#include <unordered_set>

#include "types/Message.h"
#include "types/Packet.h"

namespace SlimFly {

DimOrderRoutingAlgorithm::DimOrderRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration,
    const RoutingTable& _routingTable,
    const std::vector<u32>& _X,
    const std::vector<u32>& _X_i)
    : RoutingAlgorithm(_name, _parent, _router, _latency),
      numVcs_(router_->numVcs()), dimensionWidths_(_dimensionWidths),
      concentration_(_concentration), routingTable_(_routingTable),
      X_(_X), X_i_(_X_i) {}

DimOrderRoutingAlgorithm::~DimOrderRoutingAlgorithm() {}

void DimOrderRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {

  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->getAddress();
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress =
      _flit->getPacket()->getMessage()->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));

  std::vector<u32> dstAddr;
  for (u32 i = 1; i < destinationAddress->size(); i++) {
    dstAddr.push_back((*destinationAddress)[i]);
  }

  std::vector<RoutingTable::PathInfo> paths =
    routingTable_.getPaths(dstAddr);
  const RoutingTable::PathInfo& path =
    paths[gSim->rnd.nextU64(0, paths.size()-1)];

  // select all VCs in the output port
  for (u32 vc = 0; vc < numVcs_; vc++) {
    _response->add(path.outPortNum, vc);
  }
}

}  // namespace SlimFly