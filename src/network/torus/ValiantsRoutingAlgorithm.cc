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
#include "network/torus/ValiantsRoutingAlgorithm.h"

#include <strop/strop.h>

#include <cassert>

#include "network/torus/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Torus {

ValiantsRoutingAlgorithm::ValiantsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration, u32 _inputPort)
    : RoutingAlgorithm(_name, _parent, _router, _latency),
      numVcs_(_numVcs), dimensionWidths_(_dimensionWidths),
      concentration_(_concentration), inputPort_(_inputPort),
      inputPortDim_(computeInputPortDim(dimensionWidths_, concentration_,
                                        inputPort_)) {
  // VC set mapping:
  //  0 = stage 0 no dateline
  //  1 = stage 0 dateline
  //  2 = stage 1 no dateline
  //  3 = stage 1 dateline
  assert(numVcs_ >= 4);
}

ValiantsRoutingAlgorithm::~ValiantsRoutingAlgorithm() {}

void ValiantsRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  u32 outputPort;

  Packet* packet = _flit->getPacket();
  Message* message = packet->getMessage();

  // this is the current routers address
  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->getAddress();

  // create the routing extension if needed
  if (packet->getRoutingExtension() == nullptr) {
    // should be first router encountered
    assert(packet->getHopCount() == 1);

    // create routing extension header
    //  the extension is a vector with one dummy element then the address of the
    //  intermediate router
    std::vector<u32>* re = new std::vector<u32>(1 + routerAddress.size());
    re->at(0) = U32_MAX;  // dummy
    packet->setRoutingExtension(re);

    // random intermediate address
    for (u32 idx = 1; idx < re->size(); idx++) {
      u32 rnd = gSim->rnd.nextU64(0, dimensionWidths_.at(idx - 1) - 1);
      re->at(idx) = rnd;
    }
  }

  // get a const pointer to the address (with leading dummy)
  const std::vector<u32>* intermediateAddress =
      reinterpret_cast<const std::vector<u32>*>(packet->getRoutingExtension());

  // this is the address we are currently routing towards
  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress = message->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));
  assert(intermediateAddress->size() == destinationAddress->size());

  // determine which stage we are in based on VC set
  //  if this is a terminal port, force to stage 0
  u32 stage = (inputPortDim_ == U32_MAX) ?
      0 : ((_flit->getVc() % 4) < 2 ? 0 : 1);

  // these will be needed later
  u32 dim;
  u32 portBase = concentration_;
  bool stageTransition = false;
  const std::vector<u32>* routingTo;

  // intermediate dimension to work on
  u32 iDim;
  u32 iPortBase = concentration_;
  for (iDim = 0; iDim < routerAddress.size(); iDim++) {
    if (routerAddress.at(iDim) != intermediateAddress->at(iDim+1)) {
      break;
    }
    if (dimensionWidths_.at(iDim) == 2) {
      iPortBase += 1;
    } else {
      iPortBase += 2;
    }
  }

  // destination dimension to work on
  u32 dDim;
  u32 dPortBase = concentration_;
  for (dDim = 0; dDim < routerAddress.size(); dDim++) {
    if (routerAddress.at(dDim) != destinationAddress->at(dDim+1)) {
      break;
    }
    if (dimensionWidths_.at(dDim) == 2) {
      dPortBase += 1;
    } else {
      dPortBase += 2;
    }
  }

  // figure out with dimension of which stage we need to work on
  if (stage == 0) {
    if (iDim == routerAddress.size()) {
      // done with stage 0, go to stage 1
      dim = dDim;
      portBase = dPortBase;
      stage = 1;
      stageTransition = true;
      routingTo = destinationAddress;
    } else {
      // more work in stage 0
      dim = iDim;
      portBase = iPortBase;
      routingTo = intermediateAddress;
    }
  } else {
    // working in stage 1
    dim = dDim;
    portBase = dPortBase;
    routingTo = destinationAddress;
  }

  // test if already at destination router
  if (dim == routerAddress.size()) {
    assert(stage == 1);
    outputPort = routingTo->at(0);

    // on ejection, any dateline VcSet is ok within any stage VcSet
    for (u32 vc = 0; vc < numVcs_; vc++) {
      _response->add(outputPort, vc);
    }

    // delete the routing extension
    delete intermediateAddress;
    packet->setRoutingExtension(nullptr);
  } else {
    u32 vcSet = _flit->getVc() % 4;

    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = routingTo->at(dim + 1);
    assert(src != dst);

    // in dimension with width of 2, there is only one port
    bool right;
    if (dimensionWidths_.at(dim) == 2) {
      right = true;
    } else {
      // in torus topology, we can get to a destination in two directions,
      //  this algorithm takes the shortest path, randomized tie breaker
      u32 rightDelta = ((dst > src) ?
                        (dst - src) :
                        (dst + dimensionWidths_.at(dim) - src));
      u32 leftDelta = ((src > dst) ?
                       (src - dst) :
                       (src + dimensionWidths_.at(dim) - dst));

      // determine direction
      if (rightDelta == leftDelta) {
        right = gSim->rnd.nextBool();
      } else if (rightDelta < leftDelta) {
        right = true;
      } else {
        right = false;
      }
    }

    // choose output port, figure out next router in this dimension
    u32 next;
    if (right) {
      outputPort = portBase;
      next = (src + 1) % dimensionWidths_.at(dim);
    } else {
      outputPort = portBase + 1;
      next = src == 0 ? dimensionWidths_.at(dim) - 1 : src - 1;
    }

    // the output port is now determined, now figure out which VC(s) to use

    // reset to VC set 0 (stage 0) or 2 (stage 1) when switching dimensions or
    //  when after a stage transition. this also occurs on an injection port
    if (dim != inputPortDim_ || stageTransition) {
      vcSet = stage == 0 ? 0 : 2;
    }

    // check dateline crossing
    if (((src == 0) && (next == (dimensionWidths_.at(dim) - 1))) ||
        ((next == 0) && (src == (dimensionWidths_.at(dim) - 1)))) {
      assert(vcSet == 0 || vcSet == 2);  // can only cross once
      vcSet++;
    }

    // use VCs in the corresponding set
    for (u32 vc = vcSet; vc < numVcs_; vc += 4) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace Torus
