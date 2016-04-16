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
#include "network/torus/DimOrderRoutingAlgorithm.h"

#include <cassert>

#include "network/torus/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Torus {

DimOrderRoutingAlgorithm::DimOrderRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration, u32 _inputPort)
    : RoutingAlgorithm(_name, _parent, _router, _latency),
      numVcs_(_numVcs), dimensionWidths_(_dimensionWidths),
      concentration_(_concentration), inputPort_(_inputPort),
      inputPortDim_(computeInputPortDim(dimensionWidths_, concentration_,
                                        inputPort_)) {
  assert(numVcs_ >= 2);
}

DimOrderRoutingAlgorithm::~DimOrderRoutingAlgorithm() {}

void DimOrderRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  u32 outputPort;

  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->getAddress();
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress =
      _flit->getPacket()->getMessage()->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));

  // determine the next dimension to work on
  u32 dim;
  u32 portBase = concentration_;
  for (dim = 0; dim < routerAddress.size(); dim++) {
    if (routerAddress.at(dim) != destinationAddress->at(dim+1)) {
      break;
    }
    portBase += 2;
  }

  // test if already at destination router
  if (dim == routerAddress.size()) {
    outputPort = destinationAddress->at(0);

    // on ejection, any dateline VcSet is ok
    for (u32 vc = 0; vc < numVcs_; vc++) {
      _response->add(outputPort, vc);
    }
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = destinationAddress->at(dim + 1);
    assert(src != dst);


    // in torus topology, we can get to a destination in two directions,
    //  this algorithm takes the shortest path, randomized tie breaker
    u32 rightDelta = ((dst > src) ?
                      (dst - src) :
                      (dst + dimensionWidths_.at(dim) - src));
    u32 leftDelta = ((src > dst) ?
                     (src - dst) :
                     (src + dimensionWidths_.at(dim) - dst));

    // determine direction
    bool right;
    if (rightDelta == leftDelta) {
      right = gSim->rnd.nextBool();
    } else if (rightDelta < leftDelta) {
      right = true;
    } else {
      right = false;
    }

    // choose output port, figure out next router in this dimension
    u32 next;
    bool crossDateline;
    if (right) {
      outputPort = portBase;
      next = (src + 1) % dimensionWidths_.at(dim);
      crossDateline = next < src;
    } else {
      outputPort = portBase + 1;
      next = src == 0 ? dimensionWidths_.at(dim) - 1 : src - 1;
      crossDateline = next > src;
    }

    // the output port is now determined, now figure out which VC set to use
    assert(outputPort != inputPort_);  // this case is already checked
    u32 vcSet = _flit->getVc() % 2;

    // reset to VC set 0 when switching dimensions
    //  this also occurs on an injection port
    if (dim != inputPortDim_) {
      vcSet = 0;
    }

    // check dateline crossing
    if (crossDateline) {
      assert(vcSet == 0);  // only cross once per dim
      vcSet++;
    }

    // use VCs in the corresponding set
    for (u32 vc = vcSet; vc < numVcs_; vc += 2) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace Torus
