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
#include "network/hyperx/DimOrderRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include <unordered_set>

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

DimOrderRoutingAlgorithm::DimOrderRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _dimensionWeights,
                       _concentration, _settings) {}

DimOrderRoutingAlgorithm::~DimOrderRoutingAlgorithm() {}

void DimOrderRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  std::unordered_set<u32> outputPorts;

  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->address();
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = concentration_;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != destinationAddress->at(dim+1)) {
      break;
    }
    portBase += ((dimensionWidths_.at(dim) - 1) * dimensionWeights_.at(dim));
  }

  // test if already at destination router
  if (dim == routerAddress.size()) {
    bool res = outputPorts.insert(destinationAddress->at(0)).second;
    (void)res;
    assert(res);
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = destinationAddress->at(dim+1);
    if (dst < src) {
      dst += dimensionWidths_.at(dim);
    }
    u32 offset = (dst - src - 1) * dimensionWeights_.at(dim);
    // add all ports where the two routers are connecting

    for (u32 weight = 0; weight < dimensionWeights_.at(dim); weight++) {
      u32 outputPort = portBase + offset + weight;
      bool res = outputPorts.insert(outputPort).second;
      (void)res;
      assert(res);
    }
  }

  assert(outputPorts.size() > 0);
  if (dim != routerAddress.size()) {
    assert(outputPorts.size() == dimensionWeights_.at(dim));
  }
  for (auto it = outputPorts.cbegin(); it != outputPorts.cend(); ++it) {
    u32 outputPort = *it;
    // select all VCs in the output port
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace HyperX

registerWithObjectFactory("dimension_order", HyperX::RoutingAlgorithm,
                          HyperX::DimOrderRoutingAlgorithm,
                          HYPERX_ROUTINGALGORITHM_ARGS);
