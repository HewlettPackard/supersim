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
#include "network/torus/ValiantsRoutingAlgorithm.h"

#include <factory/Factory.h>

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace Torus {

ValiantsRoutingAlgorithm::ValiantsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _concentration, _settings) {
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

  Packet* packet = _flit->packet();
  Message* message = packet->message();

  // this is the current router's address
  // ex: [x,y,z]
  const std::vector<u32>& routerAddress = router_->address();

  // create the routing extension if needed
  if (packet->getRoutingExtension() == nullptr) {
    // should be first router encountered
    assert(packet->getHopCount() == 0);

    // create routing extension header
    //  the extension is a vector with one dummy element then the address of the
    //  intermediate router
    std::vector<u32>* re = new std::vector<u32>(1 + routerAddress.size());
    re->at(0) = U32_MAX;  // dummy
    packet->setRoutingExtension(re);

    // random intermediate address
    for (u32 idx = 1; idx < re->size(); idx++) {
      re->at(idx) = gSim->rnd.nextU64(0, dimensionWidths_.at(idx - 1) - 1);
    }
  }

  // get a const pointer to the address (with leading dummy)
  const std::vector<u32>* intermediateAddress =
      reinterpret_cast<const std::vector<u32>*>(packet->getRoutingExtension());

  // ex: [c,x,y,z]
  const std::vector<u32>* destinationAddress = message->getDestinationAddress();
  assert(routerAddress.size() == (destinationAddress->size() - 1));
  assert(intermediateAddress->size() == destinationAddress->size());

  // determine which stage we are in based on VC set
  //  if this is a terminal port, force to stage 0
  u32 stage;
  if (inputPortDim_ == U32_MAX) {
    assert(packet->getHopCount() == 0);
    stage = 0;
  } else {
    stage = (((_flit->getVc() - baseVc_) % 4) < 2) ? 0 : 1;
  }

  // intermediate dimension to work on
  u32 iDim;
  u32 iPortBase = concentration_;
  for (iDim = 0; iDim < routerAddress.size(); iDim++) {
    if (routerAddress.at(iDim) != intermediateAddress->at(iDim+1)) {
      break;
    }
    iPortBase += 2;
  }

  // destination dimension to work on
  u32 dDim;
  u32 dPortBase = concentration_;
  for (dDim = 0; dDim < routerAddress.size(); dDim++) {
    if (routerAddress.at(dDim) != destinationAddress->at(dDim+1)) {
      break;
    }
    dPortBase += 2;
  }

  // figure out which dimension of which stage we need to work on
  u32 dim;
  u32 portBase;
  bool stageTransition = false;
  const std::vector<u32>* routingTo;
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
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(outputPort, vc);
    }

    // delete the routing extension
    delete intermediateAddress;
    packet->setRoutingExtension(nullptr);
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = routingTo->at(dim + 1);
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
    assert(next != src);

    // the output port is now determined, now figure out which VC set to use
    u32 vcSet = (_flit->getVc() - baseVc_) % 4;

    // reset to VC set 0 (stage 0) or 2 (stage 1) when switching dimensions or
    //  when after a stage transition. this also occurs on an injection port
    if (dim != inputPortDim_ || stageTransition) {
      vcSet = stage == 0 ? 0 : 2;
    }

    // check dateline crossing
    if (crossDateline) {
      assert(vcSet == 0 || vcSet == 2);  // only cross once per stage per dim
      vcSet++;
    }

    // use VCs in the corresponding set
    for (u32 vc = baseVc_ + vcSet; vc < baseVc_ + numVcs_; vc += 4) {
      _response->add(outputPort, vc);
    }
  }
}

}  // namespace Torus

registerWithFactory("valiants", Torus::RoutingAlgorithm,
                    Torus::ValiantsRoutingAlgorithm,
                    TORUS_ROUTINGALGORITHM_ARGS);
