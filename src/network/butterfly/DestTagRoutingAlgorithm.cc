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
#include "network/butterfly/DestTagRoutingAlgorithm.h"

#include <factory/Factory.h>

#include <cassert>

#include "types/Packet.h"
#include "types/Message.h"

namespace Butterfly {

DestTagRoutingAlgorithm::DestTagRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _numPorts, u32 _numStages, u32 _stage,
    Json::Value _settings)
    : RoutingAlgorithm(_name, _parent, _router, _baseVc, _numVcs, _numPorts,
                       _numStages, _stage, _settings) {}

DestTagRoutingAlgorithm::~DestTagRoutingAlgorithm() {}

void DestTagRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  const std::vector<u32>* destinationAddress =
      _flit->packet()->message()->getDestinationAddress();
  assert(destinationAddress->size() == numStages_);

  // pick the output port using the "tag" in the address
  u32 outputPort = destinationAddress->at(stage_);

  // select all VCs in the output port
  for (u32 vc = baseVc_; vc < baseVc_ + numVcs_; vc++) {
    _response->add(outputPort, vc);
  }
}

}  // namespace Butterfly

registerWithFactory("dest_tag", Butterfly::RoutingAlgorithm,
                    Butterfly::DestTagRoutingAlgorithm,
                    BUTTERFLY_ROUTINGALGORITHM_ARGS);
