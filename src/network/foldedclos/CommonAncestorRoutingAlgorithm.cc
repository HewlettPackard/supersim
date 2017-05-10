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
#include "network/foldedclos/CommonAncestorRoutingAlgorithm.h"

#include <factory/Factory.h>

#include <cassert>

#include <tuple>
#include <vector>

#include "types/Packet.h"
#include "types/Message.h"

namespace FoldedClos {

CommonAncestorRoutingAlgorithm::CommonAncestorRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _numPorts, u32 _numLevels,
    u32 _inputPort, Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _numPorts,
                       _numLevels, _inputPort, _settings),
      leastCommonAncestor_(_settings["least_common_ancestor"].asBool()),
      mode_(parseMode(_settings["mode"].asString())),
      adaptive_(_settings["adaptive"].asBool()) {
  assert(!_settings["least_common_ancestor"].isNull());
  assert(!_settings["mode"].isNull());
  if ((mode_ == CommonAncestorRoutingAlgorithm::Mode::PORT) ||
      (mode_ == CommonAncestorRoutingAlgorithm::Mode::VC)) {
    assert(!_settings["adaptive"].isNull());
  }
}

CommonAncestorRoutingAlgorithm::~CommonAncestorRoutingAlgorithm() {}

void CommonAncestorRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // get addresses
  const std::vector<u32>& routerAddress = router_->address();
  assert(routerAddress.size() == 2);
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(destinationAddress->size() == numLevels_);

  u32 level = routerAddress.at(0);
  bool atTopLevel = (level == (numLevels_ - 1));
  bool movingUpward = (!atTopLevel) && (inputPort_ < (numPorts_ / 2));

  // determine if an early turn around will occur
  if (movingUpward && leastCommonAncestor_) {
    // determine if this router is an ancester of the destination
    bool isAncestor = true;
    u32 groupIndex = routerAddress.at(1);
    for (u32 i = 0; i < (numLevels_ - level - 1); i++) {
      u32 index = numLevels_ - 1 - i;
      u32 lvl = index - 1;
      u32 groupSize = (u32)pow(numPorts_ / 2, lvl);
      u32 val = groupIndex / groupSize;
      groupIndex %= groupSize;
      if (destinationAddress->at(index) != val) {
        isAncestor = false;
        break;
      }
    }
    if (isAncestor) {
      movingUpward = false;
    }
  }

  // select the outputs
  if (!movingUpward) {
    // moving downward on a deterministic path
    u32 port = destinationAddress->at(level);
    switch (mode_) {
      case CommonAncestorRoutingAlgorithm::Mode::ALL:
      case CommonAncestorRoutingAlgorithm::Mode::PORT:
        {
          // choose all VCs in the only available downward port
          for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
            _response->add(port, vc);
          }
        }
        break;

      case CommonAncestorRoutingAlgorithm::Mode::VC:
        {
          // choose a single VC in the only available downward port
          if (!adaptive_) {
            // choose a random VC
            u32 vc = gSim->rnd.nextU64(baseVc_, baseVc_ + numVcs_ - 1);
            _response->add(port, vc);
          } else {
            // choose randomly among the minimally congestion VCs
            std::vector<u32> minCongVcs;
            f64 minCong = F64_POS_INF;
            for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
              f64 cong = router_->congestionStatus(port, vc);
              if (cong < minCong) {
                minCong = cong;
                minCongVcs.clear();
              }
              if (cong <= minCong) {
                minCongVcs.push_back(vc);
              }
            }
            u32 rnd = gSim->rnd.nextU64(0, minCongVcs.size() - 1);
            u32 vc = minCongVcs.at(rnd);
            _response->add(port, vc);
          }
        }
        break;

      default:
        assert(false);
    }
  } else {
    // moving upward out any upward port
    switch (mode_) {
      case CommonAncestorRoutingAlgorithm::Mode::ALL:
        {
          // choose all VCs on all upwards ports
          for (u32 port = numPorts_ / 2; port <  numPorts_; port++) {
            for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
              _response->add(port, vc);
            }
          }
        }
        break;

      case CommonAncestorRoutingAlgorithm::Mode::PORT:
        {
          // choose all VCs on a single port
          if (!adaptive_) {
            // choose all VCs of a randomly chosen port
            u32 port = gSim->rnd.nextU64(numPorts_ / 2, numPorts_ - 1);
            for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
              _response->add(port, vc);
            }
          } else {
            // choose all VCs of a randomly chosen port among the minimally
            //  congested ports
            std::vector<u32> minCongPorts;
            f64 minCong = F64_POS_INF;
            for (u32 port = numPorts_ / 2; port < numPorts_; port++) {
              // average the port's congestion
              f64 cong = 0.0;
              for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
                cong += router_->congestionStatus(port, vc);
              }
              cong /= numVcs_;

              // compare against the current minimum
              if (cong < minCong) {
                minCong = cong;
                minCongPorts.clear();
              }
              if (cong <= minCong) {
                minCongPorts.push_back(port);
              }
            }
            u32 rnd = gSim->rnd.nextU64(0, minCongPorts.size() - 1);
            u32 port = minCongPorts.at(rnd);
            for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
              _response->add(port, vc);
            }
          }
        }
        break;

      case CommonAncestorRoutingAlgorithm::Mode::VC:
        {
          if (!adaptive_) {
            // choose a random port and vc
            u32 port = gSim->rnd.nextU64(numPorts_ / 2, numPorts_ - 1);
            u32 vc = gSim->rnd.nextU64(baseVc_, baseVc_ + numVcs_ - 1);
            _response->add(port, vc);
          } else {
            // choose randomly among the minimally congested VCs across all
            //  valid ports
            std::vector<std::tuple<u32, u32> > minCongVcs;
            f64 minCong = F64_POS_INF;
            for (u32 port = numPorts_ / 2; port < numPorts_; port++) {
              for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
                f64 cong = router_->congestionStatus(port, vc);
                if (cong < minCong) {
                  minCong = cong;
                  minCongVcs.clear();
                }
                if (cong <= minCong) {
                  minCongVcs.push_back(std::make_tuple(port, vc));
                }
              }
            }
            u32 rnd = gSim->rnd.nextU64(0, minCongVcs.size() - 1);
            std::tuple<u32, u32> portVc = minCongVcs.at(rnd);
            _response->add(std::get<0>(portVc), std::get<1>(portVc));
          }
        }
        break;

      default:
        assert(false);
    }
  }
}

CommonAncestorRoutingAlgorithm::Mode CommonAncestorRoutingAlgorithm::parseMode(
    const std::string& _mode) const {
  if (_mode == "all") {
    return CommonAncestorRoutingAlgorithm::Mode::ALL;
  } else if (_mode == "port") {
    return CommonAncestorRoutingAlgorithm::Mode::PORT;
  } else if (_mode == "vc") {
    return CommonAncestorRoutingAlgorithm::Mode::VC;
  } else {
    fprintf(stderr, "invalid mode: %s\n", _mode.c_str());
    assert(false);
  }
}

}  // namespace FoldedClos

registerWithFactory("common_ancestor", FoldedClos::RoutingAlgorithm,
                    FoldedClos::CommonAncestorRoutingAlgorithm,
                    FOLDEDCLOS_ROUTINGALGORITHM_ARGS);
