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
#include "network/dragonfly/ValiantsRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>
#include <tuple>
#include <vector>

#include "network/dragonfly/util.h"
#include "routing/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Dragonfly {

ValiantsRoutingAlgorithm::ValiantsRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    u32 _localWidth, u32 _localWeight,
    u32 _globalWidth, u32 _globalWeight,
    u32 _concentration, u32 _routerRadix, u32 _globalPortsPerRouter,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc,
                       _localWidth, _localWeight,
                       _globalWidth, _globalWeight,
                       _concentration, _routerRadix, _globalPortsPerRouter,
                       _settings),
      mode_(parseRoutingMode(_settings["mode"].asString())) {
  assert(_settings.isMember("smart_intermediate_node"));
  smartIntermediateNode_ = _settings["smart_intermediate_node"].asBool();

  // Req VCs and port bases
  rcs_ = 4;
  assert(_numVcs >= rcs_);
  localPortBase_ = concentration_;
  globalPortBase_ = concentration_ + ((localWidth_ - 1) * localWeight_);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_, true,
                                 _settings["reduction"]);
}

ValiantsRoutingAlgorithm::~ValiantsRoutingAlgorithm() {
  delete reduction_;
}

void ValiantsRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {

  // addresses
  const std::vector<u32>* sourceAddress =
      _flit->packet()->message()->getSourceAddress();
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(sourceAddress->size() == destinationAddress->size());

  Packet* packet = _flit->packet();
  // Addresses: [terminal, router, group]
  u32 thisGroup = router_->address().at(1);
  u32 thisRouter = router_->address().at(0);

  u32 destinationGroup = destinationAddress->at(2);
  u32 destinationRouter = destinationAddress->at(1);

  // create the routing extension if needed
  if (packet->getRoutingExtension() == nullptr) {
    // should be first router encountered
    assert(packet->getHopCount() == 0);
    // create routing extension header
    // random intermediate address [router, group]
    std::vector<u32>* re = new std::vector<u32>(2);
    // router
    re->at(0) = gSim->rnd.nextU64(0, localWidth_ - 1);
    // group
    re->at(1) = gSim->rnd.nextU64(0, globalWidth_ - 1);

    if (smartIntermediateNode_) {
      if (thisGroup == destinationGroup) {
        // same group
        re->at(1) = destinationGroup;
        if (thisRouter == destinationRouter) {
          // same router
          re->at(0) = destinationRouter;
        }
      }
    }
    packet->setRoutingExtension(re);
  }

  // get a const pointer to the int address [router, group]
  const std::vector<u32>* intermediateAddress =
      reinterpret_cast<const std::vector<u32>*>(packet->getRoutingExtension());

  // determine which stage we are in based on VC set
  // if this is a terminal port, force to stage 0
  u32 stage;
  bool newStage = false;
  if (inputPort_ < localPortBase_) {
    // in Terminal
    assert(packet->getHopCount() == 0);
    stage = 0;
  } else {
    stage = (vcToRc(inputVc_, rcs_) < 2) ? 0 : 1;
  }

  u32 routingToGroup;
  u32 routingToRouter;
  u32 routingToTerminal;
  if (stage == 0) {
    // to intermediate address
    assert(packet->getRoutingExtension());
    routingToGroup = intermediateAddress->at(1);
    routingToRouter = intermediateAddress->at(0);
    routingToTerminal = U32_MAX;
  } else if (stage == 1) {
    // to actual destination
    routingToGroup = destinationAddress->at(2);
    routingToRouter = destinationAddress->at(1);
    routingToTerminal = destinationAddress->at(0);
  } else {
    assert(false);
  }

  // check if at routingTo destination
  // Exit stage or network
  bool atDestination = ((thisGroup == destinationGroup) &&
                        (thisRouter == destinationRouter))? true : false;

  bool atIntermediate  = ((thisGroup == intermediateAddress->at(1)) &&
                          (thisRouter ==
                           intermediateAddress->at(0)))? true : false;

  // set stage
  if (stage == 0 && (atDestination || atIntermediate)) {
    stage = 1;
    newStage = true;
    // switch routing to destination
    routingToGroup = destinationAddress->at(2);
    routingToRouter = destinationAddress->at(1);
    routingToTerminal = destinationAddress->at(0);
    // if chose int = dst, at dst again
    atDestination = ((thisGroup == destinationGroup) &&
                     (thisRouter == destinationRouter))? true : false;
  }

  if (atDestination) {
    // exit network
    addPort(routingToTerminal, 1, U32_MAX);
    // delete the routing extension
    delete intermediateAddress;
    packet->setRoutingExtension(nullptr);
  } else {
    // moving to intermediate node or destination
    assert(newStage == atIntermediate);
    u32 globalOffset = computeOffset(thisGroup, routingToGroup,
                                     globalWidth_);

    // in Terminal
    if (inputPort_ < localPortBase_) {
      if (thisGroup == routingToGroup) {
        // Local - at destination group route locally
        assert(thisRouter != routingToRouter);  // exit detected above
        // routing to intermediate use RC 1, to dst use RC 3
        u32 rcToUse = (stage == 0) ? 1 : 3;
        addPortsToLocalRouter(thisRouter, routingToRouter, rcToUse);
      } else {
        // global - dst in remote group route globally
        routeToRemoteGroup(globalOffset, stage, newStage, thisRouter, 0, 2);
      }
      // in Local
    } else if (inputPort_ >= localPortBase_ && inputPort_ < globalPortBase_) {
      // exit detected above
      assert(thisRouter != routingToRouter || thisGroup != routingToGroup);
      if (thisGroup == routingToGroup) {
        // local - at destination group route locally - INT and DST = group
        assert(thisGroup == routingToGroup && atIntermediate);
        // route to destination with rc 3
        addPortsToLocalRouter(thisRouter, routingToRouter, 3);
      } else {
        // global - dst in remote group route globally
        routeToRemoteGroup(globalOffset, stage, newStage, thisRouter, 0, 2);
      }
      // in Global
    } else if (inputPort_ >= globalPortBase_ && inputPort_ < routerRadix_) {
      assert(newStage || thisGroup == routingToGroup);
      if (thisGroup == routingToGroup) {
        // local - at destination group route locally
        assert(thisRouter != routingToRouter);  // exit detected above
        // routing to intermediate use RC 1, to dst use RC 3
        u32 rcToUse = (stage == 0) ? 1 : 3;
        addPortsToLocalRouter(thisRouter, routingToRouter, rcToUse);
      } else {
        // global - dst in remote group route globally
        assert(atIntermediate && stage == 1);
        routeToRemoteGroup(globalOffset, stage, newStage, thisRouter, 2, 2);
      }
    } else {
      assert(false);
    }
  }

  // reduction phase
  const std::unordered_set<std::tuple<u32, u32> >* outputs =
      reduction_->reduce(nullptr);
  for (const auto& t : *outputs) {
    u32 port = std::get<0>(t);
    if (routingModeIsPort(mode_)) {
      // port mode
      // add all VCs in the specified routing class
      u32 rc = std::get<1>(t);
      if (rc != U32_MAX) {
        for (u32 vc = baseVc_ + rc; vc < baseVc_ + numVcs_; vc+= rcs_) {
          _response->add(port, vc);
        }
      } else {
        // exit - came in with rc = U32_MAX
        for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
          _response->add(port, vc);
        }
      }
    } else {
      // vc mode
      u32 vc = std::get<1>(t);
      _response->add(port, vc);
    }
  }
}

void ValiantsRoutingAlgorithm::addPort(u32 _port, u32 _hops,
                                       u32 _routingClass) {
  // add port for reduction function
  if (routingModeIsPort(mode_)) {
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, _routingClass, _hops, cong);
  } else {
    // vc mode
    if (_routingClass == U32_MAX) {
      // add all VCs in the port
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
        reduction_->add(_port, vc, _hops, cong);
      }
    } else {
      // all all VCs in the specified routing class
      for (u32 vc = baseVc_ + _routingClass; vc < baseVc_ + numVcs_;
           vc+= rcs_) {
        f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
        reduction_->add(_port, vc, _hops, cong);
      }
    }
  }
}

void ValiantsRoutingAlgorithm::routeToRemoteGroup(
    u32 _globalOffset, u32 _stage, bool _newStage, u32 _thisRouter,
    u32 _directRc, u32 _indirectRc) {
  for (u32 gw = 0; gw < globalWeight_; gw++) {
    // add ports to all global weights - all global ports are considered equal
    u32 dstGlobalPort;
    // router connected to global port
    u32 localRouter;
    u32 localRouterPort;
    computeGlobalToRouterMap(globalPortBase_,
                             globalPortsPerRouter_,
                             globalWidth_, globalWeight_,
                             localWidth_,
                             gw, _globalOffset,
                             &dstGlobalPort, &localRouter, &localRouterPort);
    u32 rcToUse = (_stage == 0) ? _directRc : _indirectRc;
    if (localRouter == _thisRouter) {
      // this router is connected to global port
      addPort(localRouterPort, 1, rcToUse);
    } else if (_stage == 0 || (_stage == 1 && _newStage)) {
      // add ports to route to local router connected to global port
      u32 localOffset = computeOffset(_thisRouter, localRouter,
                                      localWidth_);
      for (u32 lw = 0; lw < localWeight_; lw++) {
        u32 port = computeLocalSrcPort(localPortBase_, localOffset,
                                       localWeight_, lw);
        addPort(port, 1, rcToUse);
      }
    } else {
      // skipping invalid local hop
    }
  }
}

void ValiantsRoutingAlgorithm::addPortsToLocalRouter(u32 _src, u32 _dst,
                                                     u32 _routingClass) {
  u32 offset = computeOffset(_src, _dst, localWidth_);
  for (u32 localWeight = 0; localWeight < localWeight_; localWeight++) {
    u32 port = computeLocalSrcPort(localPortBase_, offset,
                                   localWeight_, localWeight);
    addPort(port, 1, _routingClass);
  }
}
}  // namespace Dragonfly

registerWithObjectFactory("valiants", Dragonfly::RoutingAlgorithm,
                          Dragonfly::ValiantsRoutingAlgorithm,
                          DRAGONFLY_ROUTINGALGORITHM_ARGS);
