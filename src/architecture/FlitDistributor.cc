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
#include "architecture/FlitDistributor.h"

#include <cassert>

FlitDistributor::FlitDistributor(
    const std::string& _name, const Component* _parent, u32 _outputs)
    : Component(_name, _parent) {
  receivers_.resize(_outputs, nullptr);
}

FlitDistributor::~FlitDistributor() {}

void FlitDistributor::setReceiver(u32 _vc, FlitReceiver* _receiver) {
  receivers_.at(_vc) = _receiver;
}

void FlitDistributor::receiveFlit(u32 _port, Flit* _flit) {
  assert(_port == 0);
  u32 vc = _flit->getVc();
  receivers_.at(vc)->receiveFlit(0, _flit);
}
