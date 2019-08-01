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
#include "network/mesh/ValiantsRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <tuple>

#include "types/Message.h"
#include "types/Packet.h"
#include "network/mesh/util.h"

namespace Mesh {

ValiantsRoutingAlgorithm::ValiantsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<u32>& _dimensionWidths,
    const std::vector<u32>& _dimensionWeights, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _dimensionWidths, _dimensionWeights,
                       _concentration, _settings),
      mode_(parseRoutingMode(_settings["mode"].asString())) {
  // VC set mapping:
  //  0 = stage 0
  //  1 = stage 1
  assert(numVcs_ >= 2);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_, false,
                                 _settings["reduction"]);
}

ValiantsRoutingAlgorithm::~ValiantsRoutingAlgorithm() {
  delete reduction_;
}

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
    stage = (_flit->getVc() - baseVc_) % 2;
  }

  // intermediate dimension to work on
  u32 iDim;
  u32 iPortBase = concentration_;
  u32 iDimWeight = U32_MAX;
  for (iDim = 0; iDim < routerAddress.size(); iDim++) {
    iDimWeight = dimensionWeights_.at(iDim);
    if (routerAddress.at(iDim) != intermediateAddress->at(iDim+1)) {
      break;
    }
    iPortBase += 2 * iDimWeight;
  }

  // destination dimension to work on
  u32 dDim;
  u32 numDimensions = dimensionWidths_.size();
  u32 dPortBase = concentration_;
  u32 dDimWeight = U32_MAX;
  for (dDim = 0; dDim < numDimensions; dDim++) {
    dDimWeight = dimensionWeights_.at(dDim);
    if (routerAddress.at(dDim) != destinationAddress->at(dDim+1)) {
      break;
    }
    dPortBase += 2 * dDimWeight;
  }

  // figure out which dimension of which stage we need to work on
  u32 dim;
  u32 portBase;
  u32 dimWeight;
  bool stageTransition = false;
  const std::vector<u32>* routingTo;
  if (stage == 0) {
    if (iDim == routerAddress.size()) {
      // done with stage 0, go to stage 1
      dim = dDim;
      portBase = dPortBase;
      dimWeight = dDimWeight;
      stage = 1;
      stageTransition = true;
      routingTo = destinationAddress;
    } else {
      // more work in stage 0
      dim = iDim;
      portBase = iPortBase;
      dimWeight = iDimWeight;
      routingTo = intermediateAddress;
    }
  } else {
    // working in stage 1
    dim = dDim;
    portBase = dPortBase;
    dimWeight = dDimWeight;
    routingTo = destinationAddress;
  }


  // create a temporary router address with a dummy concentration for use with
  //  'util.h' 'computeMinimalHops() frunction'
  std::vector<u32> tempRA = std::vector<u32>(1 + routerAddress.size());
  tempRA.at(0) = U32_MAX;  // dummy
  for (u32 ind = 1; ind < tempRA.size(); ind++) {
    tempRA.at(ind) = routerAddress.at(ind - 1);
  }

  // determine minimum number of hops to destination for reduction algorithm
  u32 hops = computeMinimalHops(&tempRA, routingTo, numDimensions,
                                dimensionWidths_);

  // the output port is now determined, now figure out which VC set to use
  u32 vcSet = (_flit->getVc() - baseVc_) % 2;

  // test if already at destination router
  if (dim == routerAddress.size()) {
    assert(stage == 1);
    outputPort = routingTo->at(0);

    // on ejection, any dateline VcSet is ok within any stage VcSet
    if (routingModeIsPort(mode_)) {
      // if routing mode is port then all vcs in the port are already added
      addPort(outputPort, hops, U32_MAX);
    } else {
      // adding each vcSet (2 for valiants) will allow for all vcs to be used
      for (u32 vcSetInd = 0; vcSetInd < 2; vcSetInd++) {
        addPort(outputPort, hops, vcSetInd);
      }
    }

    // delete the routing extension
    delete intermediateAddress;
    packet->setRoutingExtension(nullptr);
  } else {
    // more router-to-router hops needed
    u32 src = routerAddress.at(dim);
    u32 dst = routingTo->at(dim + 1);
    assert(src != dst);

    // determine direction
    bool right = dst > src;

    // choose output port, figure out next router in this dimension
    u32 next;
    if (right) {
      outputPort = portBase;
      assert(src < (dimensionWidths_.at(dim) - 1));
      next = src + 1;
    } else {
      outputPort = portBase + dimWeight;
      assert(src > 0);
      next = src - 1;
    }
    assert(next != src);

    // reset to VC set 0 (stage 0) or 1 (stage 1) after injection or a
    //  stage transition.
    if (inputPortDim_ == U32_MAX || stageTransition) {
      vcSet = stage;  // direct mapping
    }

    // add all ports connecting to the destination (based on weight)
    for (u32 wInd = 0; wInd < dimWeight; wInd++) {
      addPort(outputPort + wInd, hops, vcSet);
    }
  }

  // reduction phase
  const std::unordered_set<std::tuple<u32, u32> >* outputs =
      reduction_->reduce(nullptr);
  for (const auto& t : *outputs) {
    u32 port = std::get<0>(t);
    u32 vc = std::get<1>(t);
    if (vc == U32_MAX) {
      for (u32 vc = baseVc_ + vcSet; vc < baseVc_ + numVcs_; vc += 2) {
        _response->add(port, vc);
      }
    } else {
      _response->add(port, vc);
    }
  }
}

void ValiantsRoutingAlgorithm::addPort(u32 _port, u32 _hops, u32 vcSet) {
  if (routingModeIsPort(mode_)) {
    // add the port as a whole
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, U32_MAX, _hops, cong);
  } else {
    for (u32 vc = baseVc_ + vcSet; vc < baseVc_ + numVcs_; vc += 2) {
      f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
      reduction_->add(_port, vc, _hops, cong);
    }
  }
}

}  // namespace Mesh

registerWithObjectFactory("valiants", Mesh::RoutingAlgorithm,
                          Mesh::ValiantsRoutingAlgorithm,
                          MESH_ROUTINGALGORITHM_ARGS);
