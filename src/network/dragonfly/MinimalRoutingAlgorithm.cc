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
#include "network/dragonfly/MinimalRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>

#include <cassert>
#include <tuple>
#include <vector>

#include "network/dragonfly/util.h"
#include "types/Message.h"
#include "types/Packet.h"

namespace Dragonfly {

MinimalRoutingAlgorithm::MinimalRoutingAlgorithm(
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
  assert(_settings.isMember("randomized_global"));
  randomizedGlobal_ = _settings["randomized_global"].asBool();

  // Req VCs and port bases
  rcs_ = 2;
  assert(_numVcs >= rcs_);
  localPortBase_ = concentration_;
  globalPortBase_ = concentration_ + ((localWidth_ - 1) * localWeight_);

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_, true,
                                 _settings["reduction"]);
}

MinimalRoutingAlgorithm::~MinimalRoutingAlgorithm() {
  delete reduction_;
}

void MinimalRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // addresses
  const std::vector<u32>* sourceAddress =
      _flit->packet()->message()->getSourceAddress();
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(sourceAddress->size() == destinationAddress->size());

  // topology info
  u32 thisGroup = router_->address().at(1);
  u32 destinationGroup = destinationAddress->at(2);
  u32 globalOffset = computeOffset(thisGroup, destinationGroup, globalWidth_);

  u32 thisRouter = router_->address().at(0);
  u32 destinationRouter = destinationAddress->at(1);
  u32 destinationTerminal = destinationAddress->at(0);

  // in Terminal
  if (inputPort_ < localPortBase_) {
    if (thisGroup == destinationGroup) {
      // destination in current group
      if (thisRouter == destinationRouter) {
        // exit - to terminal
        addPort(destinationTerminal, 1, U32_MAX);
      } else {
        // local - route to destination router in this group
        addPortsToLocalRouter(thisRouter, destinationRouter, 0);
      }
    } else {
      // global - route to dst group
      routeToRemoteGroup(globalOffset, thisRouter, 0, true);
    }
    // in Local
  } else if (inputPort_ >= localPortBase_ && inputPort_ < globalPortBase_) {
    if (thisRouter == destinationRouter && thisGroup == destinationGroup) {
      // exit - to terminal
      addPort(destinationTerminal, 1, U32_MAX);
    } else {
      // global - hop to dst group (no local hop allowed again)
      assert(thisGroup != destinationGroup);
      routeToRemoteGroup(globalOffset, thisRouter, 0, false);
    }
    // in Global
  } else if (inputPort_ >= globalPortBase_ && inputPort_ < routerRadix_) {
    assert(thisGroup == destinationGroup);
    if (thisRouter == destinationRouter) {
      // exit - to terminal
      addPort(destinationTerminal, 1, U32_MAX);
    } else {
      // local - route to destination router in this group
      addPortsToLocalRouter(thisRouter, destinationRouter, 1);
    }
  } else {
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

void MinimalRoutingAlgorithm::addPort(u32 _port, u32 _hops, u32 _routingClass) {
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

void MinimalRoutingAlgorithm::routeToRemoteGroup(
    u32 _globalOffset, u32 _thisRouter, u32 _Rc, bool _indirectGlobals) {
  // route to take a global hop
  std::unordered_set<u32> setOfPorts;
  // route to attached global channel only
  for (u32 gw = 0; gw < globalWeight_; gw++) {
    u32 dstGlobalPort;
    // router connected to global port
    u32 localRouter;
    u32 localPort;
    computeGlobalToRouterMap(globalPortBase_,
                             globalPortsPerRouter_,
                             globalWidth_, globalWeight_,
                             localWidth_,
                             gw, _globalOffset,
                             &dstGlobalPort, &localRouter, &localPort);
    if (localRouter == _thisRouter) {
      // only add global hops connected to this router
      if (!randomizedGlobal_) {
        // not random - add all connected global ports
        addPort(localPort, 1, _Rc);
      } else {
        // random selection - add to set
        setOfPorts.insert(localPort);
      }
    } else if (_indirectGlobals) {
      assert(inputPort_ < localPortBase_);  // only allowed for inTerminal
      // add ports to route to local router connected to global port
      u32 offset = computeOffset(_thisRouter, localRouter, localWidth_);
      for (u32 lw = 0; lw < localWeight_; lw++) {
        u32 port = computeLocalSrcPort(localPortBase_, offset,
                                       localWeight_, lw);
        if (!randomizedGlobal_) {
          // not random - add all connected global ports
          addPort(port, 1, _Rc);
        } else {
          // random selection - add to set
          setOfPorts.insert(port);
        }
      }
    }
  }
  if (randomizedGlobal_) {
    // randomly select one of the global ports connected to router
    u32 port = gSim->rnd.retrieve(&setOfPorts);
    addPort(port, 1, _Rc);
  }
}

void MinimalRoutingAlgorithm::addPortsToLocalRouter(u32 _src, u32 _dst,
                                                    u32 _routingClass) {
  u32 offset = computeOffset(_src, _dst, localWidth_);
  for (u32 localWeight = 0; localWeight < localWeight_; localWeight++) {
    u32 port = computeLocalSrcPort(localPortBase_, offset,
                                   localWeight_, localWeight);
    addPort(port, 1, _routingClass);
  }
}
}  // namespace Dragonfly

registerWithObjectFactory("minimal", Dragonfly::RoutingAlgorithm,
                          Dragonfly::MinimalRoutingAlgorithm,
                          DRAGONFLY_ROUTINGALGORITHM_ARGS);
