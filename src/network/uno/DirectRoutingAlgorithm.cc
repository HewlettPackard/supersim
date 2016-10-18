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
#include "network/uno/DirectRoutingAlgorithm.h"

#include <cassert>

#include "types/Packet.h"
#include "types/Message.h"

namespace Uno {

DirectRoutingAlgorithm::DirectRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _baseVc, u32 _numVcs, u32 _concentration,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _latency, _baseVc, _numVcs),
      concentration_(_concentration),
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
      f64 cong = router_->congestionStatus(outputPort, vc);
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
