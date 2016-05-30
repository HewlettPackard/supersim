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
#include "network/foldedclos/McaRoutingAlgorithm.h"

#include <cassert>
#include <vector>

#include "types/Packet.h"
#include "types/Message.h"

namespace FoldedClos {

McaRoutingAlgorithm::McaRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u64 _latency, u32 _numVcs, u32 _numPorts, u32 _numLevels, u32 _level,
    u32 _inputPort, bool _adaptive)
    : RoutingAlgorithm(_name, _parent, _router, _latency),
      numVcs_(_numVcs), numPorts_(_numPorts), numLevels_(_numLevels),
      level_(_level), inputPort_(_inputPort), adaptive_(_adaptive) {}

McaRoutingAlgorithm::~McaRoutingAlgorithm() {}

void McaRoutingAlgorithm::processRequest(
    Flit* _flit, RoutingAlgorithm::Response* _response) {
  u32 outputPort = U32_MAX;

  bool atTopLevel = (level_ == (numLevels_ - 1));
  bool movingUpward = (inputPort_ < (numPorts_ / 2));

  // up facing ports of top level are disconnected, make sure there is no funny
  //  business going on.
  assert(!(atTopLevel && !movingUpward));

  const std::vector<u32>& routerAddress = router_->getAddress();
  assert(routerAddress.size() == numLevels_);
  const std::vector<u32>* destinationAddress =
      _flit->getPacket()->getMessage()->getDestinationAddress();
  assert(destinationAddress->size() == numLevels_);

  if (movingUpward && !atTopLevel) {
    // the packet needs continue upward
    if (!adaptive_) {
      // choose a random upward output port
      outputPort = gSim->rnd.nextU64(numPorts_ / 2, numPorts_ - 1);
    } else {
      // choose the port with the most availability
      f64 maxAvailability = F64_NEG_INF;
      for (u32 port = numPorts_ / 2; port < numPorts_; port++) {
        f64 availability = 0.0;
        for (u32 vc = 0; vc < numVcs_; vc++) {
          u32 vcIdx = router_->vcIndex(port, vc);
          availability += router_->congestionStatus(vcIdx);
        }
        availability /= numVcs_;  // average
        if (availability > maxAvailability) {
          maxAvailability = availability;
          outputPort = port;
        }
      }
    }
  } else {
    // when moving down the tree, output port is a simple lookup into the
    //  destination address vector
    outputPort = destinationAddress->at(level_);
  }

  // select all VCs in the output port
  assert(outputPort != U32_MAX);
  for (u32 vc = 0; vc < numVcs_; vc++) {
    _response->add(outputPort, vc);
  }
}

}  // namespace FoldedClos
