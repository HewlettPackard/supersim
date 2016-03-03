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
#include "network/hyperx/AnyInjectionAlgorithm.h"

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

AnyInjectionAlgorithm::AnyInjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u64 _latency, u32 _numVcs)
    : InjectionAlgorithm(_name, _parent, _interface, _latency),
      numVcs_(_numVcs) {}

AnyInjectionAlgorithm::~AnyInjectionAlgorithm() {}

void AnyInjectionAlgorithm::processRequest(
    Message* _message, InjectionAlgorithm::Response* _response) {
  // use all VCs
  for (u32 vc = 0; vc < numVcs_; vc++) {
    _response->add(vc);
  }
}

}  // namespace HyperX
