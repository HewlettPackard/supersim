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
#include "network/uno/DirectRoutingAlgorithm.h"

#include <factory/Factory.h>

#include <cassert>

#include <vector>

#include "types/Packet.h"
#include "types/Message.h"

namespace Uno {

DirectRoutingAlgorithm::DirectRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _inputPort,
                       _inputVc, _concentration, _settings),
      adaptive_(_settings["adaptive"].asBool()) {
  assert(!_settings["adaptive"].isNull());
}

DirectRoutingAlgorithm::~DirectRoutingAlgorithm() {}

void DirectRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  // direct route to destination
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  u32 outputPort = destinationAddress->at(0);
  assert(outputPort < concentration_);

  if (!adaptive_) {
    // select all VCs in the output port
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      _response->add(outputPort, vc);
    }
  } else {
    // select all minimally congested VCs
    std::vector<u32> minCongVcs;
    f64 minCong = F64_POS_INF;
    for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
      f64 cong = router_->congestionStatus(inputPort_, inputVc_, outputPort,
                                           vc);
      if (cong < minCong) {
        minCong = cong;
        minCongVcs.clear();
      }
      if (cong <= minCong) {
        minCongVcs.push_back(vc);
      }
      for (u32 vc : minCongVcs) {
        _response->add(outputPort, vc);
      }
    }
  }
}

}  // namespace Uno

registerWithFactory("direct", Uno::RoutingAlgorithm,
                    Uno::DirectRoutingAlgorithm, UNO_ROUTINGALGORITHM_ARGS);
