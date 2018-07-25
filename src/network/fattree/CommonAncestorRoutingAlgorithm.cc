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
#include "network/fattree/CommonAncestorRoutingAlgorithm.h"

#include <factory/ObjectFactory.h>
#include <strop/strop.h>

#include <cassert>

#include <tuple>
#include <vector>

#include "network/fattree/util.h"
#include "types/Packet.h"
#include "types/Message.h"

namespace FatTree {

CommonAncestorRoutingAlgorithm::CommonAncestorRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    const std::vector<std::tuple<u32, u32, u32> >* _radices,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _radices, _settings),
      mode_(parseRoutingMode(_settings["mode"].asString())),
      leastCommonAncestor_(_settings["least_common_ancestor"].asBool()),
      deterministic_(_settings["deterministic"].asBool()),
      random_(gSim->rnd.nextU64(0, 0xdeadbeef)) {
  assert(!_settings["least_common_ancestor"].isNull());
  assert(!_settings["mode"].isNull());
  assert(!_settings["deterministic"].isNull());

  // create the reduction
  reduction_ = Reduction::create("Reduction", this, _router, mode_,
                                 false, _settings["reduction"]);
}

CommonAncestorRoutingAlgorithm::~CommonAncestorRoutingAlgorithm() {
  delete reduction_;
}

void CommonAncestorRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // addresses
  const std::vector<u32>* sourceAddress =
      _flit->packet()->message()->getSourceAddress();
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(sourceAddress->size() == destinationAddress->size());

  // topology info
  const u32 level = router_->address().at(0);
  const u32 numLevels = sourceAddress->size();
  const u32 downPorts = std::get<0>(radices_->at(level));
  const u32 upPorts = std::get<1>(radices_->at(level));

  // current location info
  bool atTopLevel = (level == (numLevels - 1));
  bool movingUpward = (!atTopLevel) && (inputPort_ < downPorts);
  u32 lca = leastCommonAncestor(sourceAddress, destinationAddress);
  // u64 uniqueId = _flit->packet()->message()->getTransaction();
  // std::vector<u32> thisRouter = router_->address();
  // std::vector<u32> wanted = {1, 3};

  // determine if an early turn around will occur
  if (movingUpward && leastCommonAncestor_) {
    // determine if this router is an ancester of the destination
    assert(lca >= level);
    if (lca == level) {
      movingUpward = false;
    }
  }

  // determine hop to destination
  u32 hops;
  if (movingUpward) {
    assert(level < numLevels - 1);
    if (leastCommonAncestor_) {
      hops = (lca + 1) + (lca - level);
    } else {
      hops = numLevels + (numLevels - level - 1);
    }
  } else {
    hops = level + 1;
  }

  // select the outputs
  if (!movingUpward) {
    // moving downward on a deterministic path
    u32 port = destinationAddress->at(level);
    addPort(port, hops);
  } else {
    // moving upward
    if (deterministic_) {
      // hash the source and destination to find the one path up
      u32 sourceId = _flit->packet()->message()->getSourceId();
      u32 destinationId = _flit->packet()->message()->getDestinationId();
      u32 hash = hasher_(std::make_tuple(sourceId, destinationId, random_));
      u32 port = downPorts + (hash % upPorts);
      addPort(port, hops);
    } else {
      // choose all upward ports
      for (u32 up = 0; up < upPorts; up++) {
        u32 port = downPorts + up;
        addPort(port, hops);
      }
    }
  }

  // reduction phase
  const std::unordered_set<std::tuple<u32, u32> >* outputs =
      reduction_->reduce(nullptr);
  for (const auto& t : *outputs) {
    u32 port = std::get<0>(t);
    u32 vc = std::get<1>(t);
    if (vc == U32_MAX) {
      for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
        _response->add(port, vc);
      }
    } else {
      _response->add(port, vc);
    }
  }
}

void CommonAncestorRoutingAlgorithm::addPort(u32 _port, u32 _hops) {
  if (routingModeIsPort(mode_)) {
    // add the port as a whole
    f64 cong = portCongestion(mode_, router_, inputPort_, inputVc_, _port);
    reduction_->add(_port, U32_MAX, _hops, cong);
  } else {
    // add all VCs in the port
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      f64 cong = router_->congestionStatus(inputPort_, inputVc_, _port, vc);
      reduction_->add(_port, vc, _hops, cong);
    }
  }
}

}  // namespace FatTree

registerWithObjectFactory("common_ancestor", FatTree::RoutingAlgorithm,
                          FatTree::CommonAncestorRoutingAlgorithm,
                          FATTREE_ROUTINGALGORITHM_ARGS);
