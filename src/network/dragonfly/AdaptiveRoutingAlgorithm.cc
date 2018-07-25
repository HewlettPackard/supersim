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
#include "network/dragonfly/AdaptiveRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <tuple>
#include <vector>

#include "network/dragonfly/util.h"
#include "routing/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Dragonfly {
// consts for map
static const u32 kSrc1 = 0;
static const u32 kSrc2 = 1;
static const u32 kGlobalNonMin = 2;
static const u32 kInter1 = 3;
static const u32 kInter2 = 4;
static const u32 kGlobalMin = 5;
static const u32 kDst1 = 6;

std::vector<u32> AdaptiveRoutingAlgorithm::createRoutingClasses(
    Json::Value _settings) {
  assert(_settings.isMember("progressive_adaptive"));
  bool par = _settings["progressive_adaptive"].asBool();
  assert(_settings.isMember("valiant_node"));
  bool valn = _settings["valiant_node"].asBool();
  // S1, S2, Global non-min, I1, I2, Global min, D1
  if (par && valn) {
    return std::vector<u32>({0, 1, 1, 2, 3, 3, 4});
  } else if (par && !valn) {
    return std::vector<u32>({0, 1, 1, U32_MAX, 2, 2, 3});
  } else if (!par && valn) {
    return std::vector<u32>({U32_MAX, 0, 0, 1, 2, 2, 3});
  } else if (!par && !valn) {
    return std::vector<u32>({U32_MAX, 0, 0, U32_MAX, 1, 1, 2});
  } else {
    assert(false);
  }
}

AdaptiveRoutingAlgorithm::AdaptiveRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    u32 _localWidth, u32 _localWeight,
    u32 _globalWidth, u32 _globalWeight,
    u32 _concentration, u32 _routerRadix,
    u32 _globalPortsPerRouter,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc,
                       _localWidth, _localWeight,
                       _globalWidth, _globalWeight,
                       _concentration, _routerRadix,
                       _globalPortsPerRouter,
                       _settings),
      progressiveAdaptive_(_settings["progressive_adaptive"].asBool()),
      valiantNode_(_settings["valiant_node"].asBool()),
      routingClasses_(createRoutingClasses(_settings)),
      mode_(parseRoutingMode(_settings["mode"].asString())) {
  // checks
  assert(_settings.isMember("progressive_adaptive"));
  assert(_settings.isMember("valiant_node"));

  // Req VCs and port bases
  rcs_ = 3 + progressiveAdaptive_ + valiantNode_;
  assert(_numVcs >= rcs_);
  localPortBase_ = concentration_;
  globalPortBase_ = concentration_ + ((localWidth_ - 1) * localWeight_);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_, true,
                                 _settings["reduction"]);
}

AdaptiveRoutingAlgorithm::~AdaptiveRoutingAlgorithm() {
  delete reduction_;
}

void AdaptiveRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {

  // addresses
  const std::vector<u32>* sourceAddress =
      _flit->packet()->message()->getSourceAddress();
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(sourceAddress->size() == destinationAddress->size());

  // topology info
  u32 sourceGroup = sourceAddress->at(2);

  u32 thisRouter = router_->address().at(0);
  u32 thisGroup = router_->address().at(1);

  u32 destinationRouter = destinationAddress->at(1);
  u32 destinationGroup = destinationAddress->at(2);
  u32 destinationTerminal = destinationAddress->at(0);

  u32 destinationGlobalOffset = computeOffset(thisGroup, destinationGroup,
                                              globalWidth_);

  if (inputPort_ < localPortBase_) {
    // in TERMINAL
    assert(thisGroup == sourceGroup);
    if (thisGroup != destinationGroup) {
      // GLOBAL hop - ugal - add all global ports (min/nonmin)
      // at src group
      if (progressiveAdaptive_) {
        addGlobalPorts(thisRouter, true, true, true, true,
                       destinationGlobalOffset,
                       kGlobalMin, kGlobalNonMin, kSrc1, kSrc1);
      } else {
        addGlobalPorts(thisRouter, true, true, true, true,
                       destinationGlobalOffset,
                       kGlobalMin, kGlobalNonMin, kSrc2, kSrc2);
      }
    } else if (thisGroup == destinationGroup) {
      // LOCAL - routing only - no global routing
      // in at dst group, srcgroup == dst group
      if (thisRouter == destinationRouter) {
        // exit
        addPort(destinationTerminal, 1, U32_MAX);
      } else {
        // add all local ports aimed to destination router (min/nonmin)
        addPortsToLocalRouter(thisRouter, destinationRouter,
                              false, kDst1, kSrc2);
      }
    } else {
      // can't come in through terminal at intermediate group
      assert(false);
    }

  } else if (inputPort_ < globalPortBase_ && inputPort_ >= localPortBase_) {
    // in LOCAL
    if (thisGroup == sourceGroup && thisGroup != destinationGroup) {
      // GLOBAL hop
      // at source group but not destination group
      if (vcToRc(inputVc_, rcs_) == routingClasses_.at(kSrc1)) {
        // in with S1
        // first hop of PAR done - taking global hop but can chose diff global
        assert(progressiveAdaptive_);
        // can use attached or peer attached global ports
        addGlobalPorts(thisRouter, true, true, true, true,
                       destinationGlobalOffset,
                       kGlobalMin, kGlobalNonMin, kSrc2, kSrc2);
      } else {
        // in with S2
        // already used PAR or not PAR active
        // can ONLY use attached global ports
        addGlobalPorts(thisRouter, true, true, false, false,
                       destinationGlobalOffset,
                       kGlobalMin, kGlobalNonMin, U32_MAX, U32_MAX);
      }
    } else if (thisGroup != sourceGroup && thisGroup != destinationGroup) {
      // at intermediate group
      if (vcToRc(inputVc_, rcs_) == routingClasses_.at(kInter1)) {
        assert(valiantNode_);
        // in with I1
        // (int direct) add all local ports aimed to all minimal globals
        addGlobalPorts(thisRouter, true, false, true, false,
                       destinationGlobalOffset,
                       kGlobalMin, U32_MAX, kInter2, U32_MAX);
      } else if (vcToRc(inputVc_, rcs_) == routingClasses_.at(kInter2)) {
        // in with I2
        // add all connected minimal globals as min - use global minimal
        addGlobalPorts(thisRouter, true, false, false, false,
                       destinationGlobalOffset,
                       kGlobalMin, U32_MAX, U32_MAX, U32_MAX);
      } else {
        assert(false);
      }
    } else if (thisGroup != sourceGroup && thisGroup == destinationGroup) {
      // at destination group but not src group
      // exit
      assert(thisRouter == destinationRouter);
      addPort(destinationTerminal, 1, U32_MAX);
    } else if (thisGroup == sourceGroup && thisGroup == destinationGroup) {
      // src group == dst group
      if (vcToRc(inputVc_, rcs_) == routingClasses_.at(kSrc2)) {
        // in with S2 route to dst with dst 1
        addPortsToLocalRouter(thisRouter, destinationRouter,
                              true, kDst1, U32_MAX);
      } else if (vcToRc(inputVc_, rcs_) == routingClasses_.at(kDst1)) {
        // in Dst1 exit
        assert(thisRouter == destinationRouter);
        addPort(destinationTerminal, 1, U32_MAX);
      } else {
        // coming in with the wrong VC!
        assert(false);
      }
    } else {
      assert(false);
    }
  } else if (inputPort_ >= globalPortBase_ && inputPort_ < routerRadix_) {
    // IN GLOBAL
    if (thisGroup != destinationGroup) {
      assert(vcToRc(inputVc_, rcs_) == routingClasses_.at(kGlobalNonMin));
      // at intermediate group - in global non-minimal
      if (valiantNode_) {
        // WE AREN'T HANDLING SHORT CUT
        // lie to the function to use all local peers
        addGlobalPorts(thisRouter, true, false, true, true,
                       destinationGlobalOffset,
                       kGlobalMin, U32_MAX, kInter1, kInter1);
      } else {
        // not valn
        // int direct add all local ports aimed to all minimal globals
        addGlobalPorts(thisRouter, true, false, true, false,
                       destinationGlobalOffset,
                       kGlobalMin, U32_MAX, kInter2, U32_MAX);
      }
    } else if (thisGroup == destinationGroup) {
      // at destination group - in global minimal
      assert(vcToRc(inputVc_, rcs_) == routingClasses_.at(kGlobalMin));
      if (thisRouter == destinationRouter) {
        // exit
        addPort(destinationTerminal, 1, U32_MAX);
      } else {
        // add all local ports aimed to dst router
        addPortsToLocalRouter(thisRouter, destinationRouter,
                              true, kDst1, U32_MAX);
      }
    } else {
      // should not be at src group after global hop
      assert(false);
    }
  } else {
    // only in terminal, local, global
    assert(false);
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
        for (u32 vc = baseVc_ + rc; vc < baseVc_ + numVcs_; vc += rcs_) {
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

void AdaptiveRoutingAlgorithm::addPort(
    u32 _port, u32 _hops, u32 _routingClass) {
  // add port for reduction function
  if (routingModeIsPort(mode_)) {
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, _routingClass, _hops, cong);
  } else {
    if (_routingClass == U32_MAX) {
      // add all VCs in the port
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
        reduction_->add(_port, vc, _hops, cong);
      }
    } else {
      // all all VCs in the specified routing class
      for (u32 vc = baseVc_ + _routingClass; vc < baseVc_ + numVcs_;
           vc += rcs_) {
        f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
        reduction_->add(_port, vc, _hops, cong);
      }
    }
  }
}

// LOCAL
void AdaptiveRoutingAlgorithm::addPortsToLocalRouter(
    u32 _src, u32 _dst, bool _minimalOnly, u32 _minRc, u32 _nonminRc) {
  // ports to local router [minimalOnly = true] only add minimal routes
  u32 minOffset = computeOffset(_src, _dst, localWidth_);

  u32 offsetStart = _minimalOnly ? minOffset : 1;
  u32 offsetStop = _minimalOnly ? minOffset + 1 : localWidth_;

  for (u32 lOffset = offsetStart; lOffset < offsetStop; lOffset++) {
    bool isMinimal = lOffset == minOffset;
    for (u32 lWeight = 0; lWeight < localWeight_; lWeight++) {
      u32 port = computeLocalSrcPort(localPortBase_, lOffset,
                                     localWeight_, lWeight);

      u32 rc = isMinimal ? _minRc : _nonminRc;
      u32 hops = isMinimal ? 1 : 2;
      addPort(port, hops, routingClasses_.at(rc));
    }
  }
}

// GLOBAL
void AdaptiveRoutingAlgorithm::addGlobalPorts(
    u32 _thisRouter, bool _attachedMin, bool _attachedNonMin,
    bool _peerMin, bool _peerNonMin, u32 _dstGlobalOffset,
    u32 _minGlobalRc, u32 _nonminGlobalRc,
    u32 _minLocalRc, u32 _nonminLocalRc) {
  // add all global ports (as ports of this routers)
  // options: attached min or attached nonmin | peer min and peer nonmin

  assert(_dstGlobalOffset > 0 && _dstGlobalOffset < globalWidth_);
  bool minimalOnly = !_attachedNonMin && !_peerNonMin;
  u32 offsetStart = minimalOnly ? _dstGlobalOffset : 1;
  u32 offsetStop = minimalOnly ? _dstGlobalOffset + 1 : globalWidth_;

  for (u32 gOffset = offsetStart; gOffset < offsetStop; gOffset++) {
    bool isMinimal = gOffset == _dstGlobalOffset;
    for (u32 gWeight = 0; gWeight < globalWeight_; gWeight++) {
      u32 localRouter;
      u32 thisLocalPort;
      u32 thisGlobalPort;
      computeGlobalToRouterMap(globalPortBase_, globalPortsPerRouter_,
                               globalWidth_, globalWeight_,
                               localWidth_, gWeight, gOffset, &thisGlobalPort,
                               &localRouter, &thisLocalPort);

      if (localRouter == _thisRouter) {
        if ((isMinimal && _attachedMin) || (!isMinimal && _attachedNonMin)) {
          // add port directly connected
          u32 rc = isMinimal ? _minGlobalRc : _nonminGlobalRc;
          u32 hops = isMinimal ? 1 : 2;
          addPort(thisLocalPort, hops, routingClasses_.at(rc));
        }
      } else {  // local peer router
        if ((isMinimal && _peerMin) || (!isMinimal && _peerNonMin)) {
          // add ports to route to local router connected to global port
          u32 rc = isMinimal ? _minLocalRc : _nonminLocalRc;
          u32 hops = isMinimal ? 1 : 2;
          u32 lOffset = computeOffset(_thisRouter, localRouter, localWidth_);
          for (u32 lWeight = 0; lWeight < localWeight_; lWeight++) {
            u32 port = computeLocalSrcPort(localPortBase_, lOffset,
                                           localWeight_, lWeight);
            addPort(port, hops, routingClasses_.at(rc));
          }
        }
      }
    }
  }
}
}  // namespace Dragonfly

registerWithObjectFactory("adaptive", Dragonfly::RoutingAlgorithm,
                          Dragonfly::AdaptiveRoutingAlgorithm,
                          DRAGONFLY_ROUTINGALGORITHM_ARGS);
