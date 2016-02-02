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
#include "network/torus/DimOrderRoutingFunction.h"

#include <strop/strop.h>

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace Torus {

DimOrderRoutingFunction::DimOrderRoutingFunction(
    const std::string& _name, const Component* _parent, u64 _latency,
    Router* _router, u32 _numVcs, std::vector<u32> _dimensionWidths,
    u32 _concentration, u32 _inputPort)
    : RoutingFunction(_name, _parent, _latency), router_(_router),
      numVcs_(_numVcs), dimensionWidths_(_dimensionWidths),
      concentration_(_concentration), inputPort_(_inputPort) {
  // determine if this input port is directly connected to a terminal
  isTerminalPort_ = inputPort_ < concentration_;
  // determine which network dimension this port is attached to
  inputPortDim_ = U32_MAX;  // invalid
  if (!isTerminalPort_) {
    u32 port = inputPort_ - concentration_;
    for (u32 dim = 0; dim < dimensionWidths_.size(); dim++) {
      u32 dimWidth = dimensionWidths_.at(dim);
      u32 dimPorts = dimWidth == 2 ? 1 : 2;
      if (port <= dimPorts) {
        inputPortDim_ = dim;
        break;
      } else {
        port -= dimPorts;
      }
    }
    assert(inputPortDim_ != U32_MAX);
  }
}

DimOrderRoutingFunction::~DimOrderRoutingFunction() {}

void DimOrderRoutingFunction::processRequest(
    Flit* _flit, RoutingFunction::Response* _response) {
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
    if (dimensionWidths_.at(dim) == 2) {
      portBase += 1;
    } else {
      portBase += 2;
    }
  }

  // test if already at destination router
  if (dim == routerAddress.size()) {
    outputPort = destinationAddress->at(0);
    // use all VCs on egress
    for (u32 i = 0; i < numVcs_; i++) {
      _response->add(outputPort, i);
    }
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = destinationAddress->at(dim + 1);
    assert(src != dst);
    // in dimension with width of 2, there is only one port
    if (dimensionWidths_.at(dim) == 2) {
      outputPort = portBase;
    } else {
      // in torus topology, we can get to a destination in two directions,
      //  this algorithm takes the shortest path, randomized tie breaker
      u32 rightDelta = (dst > src) ?
                       (dst - src) :
                       (dst + dimensionWidths_.at(dim) - src);
      u32 leftDelta = (src > dst) ?
                      (src - dst) :
                      (src + dimensionWidths_.at(dim) - dst);
      if (rightDelta == leftDelta) {
        outputPort = portBase + gSim->rnd.nextU64(0, 1);
      } else if (rightDelta < leftDelta) {
        outputPort = portBase;
      } else {
        outputPort = portBase + 1;
      }
    }

    // the output port is now determined, now figure out which VC(s) to use
    assert(outputPort != inputPort_);  // this case is already checked

    u32 vcSet;
    if (dim != inputPortDim_) {
      // reset to VC set 0 when switching dimensions
      vcSet = 0;
    } else {
      u32 currentVcSet = _flit -> getVc() % 2;

      // in same dimension, detect dateline crossing or stay in same set
      if (((src == 0) && (dst == (dimensionWidths_.at(dim) - 1))) ||
          ((dst == 0) && (src == (dimensionWidths_.at(dim) - 1)))) {
        assert(currentVcSet == 0);  // can only cross once
        vcSet = 1;
      } else {
        vcSet = currentVcSet;
      }
    }

    // use VCs in the corresponding set
    for (u32 vc = vcSet; vc < numVcs_; vc += 2) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace Torus
