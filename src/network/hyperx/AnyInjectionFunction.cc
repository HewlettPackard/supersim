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
#include "network/hyperx/AnyInjectionFunction.h"

#include <cassert>

#include "types/Message.h"
#include "types/Packet.h"

namespace HyperX {

AnyInjectionFunction::AnyInjectionFunction(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u64 _latency)
    : InjectionFunction(_name, _parent, _interface, _latency) {}

AnyInjectionFunction::~AnyInjectionFunction() {}

void AnyInjectionFunction::processRequest(
    Message* _message, InjectionFunction::Response* _response) {
  u32 numVcs = interface_->numVcs();

  // use all VCs
  for (u32 vc = 0; vc < numVcs; vc++) {
    _response->add(vc);
  }
}

}  // namespace HyperX
