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
#include "network/uno/DirectRoutingFunction.h"

#include <strop/strop.h>

#include <cassert>

#include "types/Packet.h"
#include "types/Message.h"

namespace Uno {

DirectRoutingFunction::DirectRoutingFunction(
    const std::string& _name, const Component* _parent, u64 _latency,
    Router* _router, u32 _numVcs, u32 _concentration, bool _allVcs)
    : RoutingFunction(_name, _parent, _latency), router_(_router),
      numVcs_(_numVcs), concentration_(_concentration), allVcs_(_allVcs) {
}

DirectRoutingFunction::~DirectRoutingFunction() {}

void DirectRoutingFunction::processRequest(
    Flit* _flit, RoutingFunction::Response* _response) {
  // direct route to destination
  const std::vector<u32>* destinationAddress =
      _flit->getPacket()->getMessage()->getDestinationAddress();
  u32 outputPort = destinationAddress->at(0);
  assert(outputPort < concentration_);

  if (allVcs_) {
    // select all VCs in the output port
    for (u32 vc = 0; vc < numVcs_; vc++) {
      _response->add(outputPort, vc);
    }
  } else {
    // use the current VC
    _response->add(outputPort, _flit->getVc());
  }
}

}  // namespace Uno
