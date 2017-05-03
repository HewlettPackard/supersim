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
#include "network/RoutingAlgorithm_TEST.h"

#include <cassert>

RoutingAlgorithmTestRouter::RoutingAlgorithmTestRouter(
    const std::string& _name, u32 _numPorts, u32 _numVcs)
    : Router(_name, nullptr, nullptr, 0, {}, _numPorts, _numVcs,
             nullptr, Json::Value()) {}

RoutingAlgorithmTestRouter::~RoutingAlgorithmTestRouter() {}

void RoutingAlgorithmTestRouter::setInputChannel(u32 _port, Channel* _channel) {
  assert(false);
}

Channel* RoutingAlgorithmTestRouter::getInputChannel(u32 _port) const {
  assert(false);
}

void RoutingAlgorithmTestRouter::setOutputChannel(
    u32 _port, Channel* _channel) {
  assert(false);
}

Channel* RoutingAlgorithmTestRouter::getOutputChannel(u32 _port) const {
  assert(false);
}

void RoutingAlgorithmTestRouter::receiveFlit(u32 _port, Flit* _flit) {
  assert(false);
}

void RoutingAlgorithmTestRouter::receiveCredit(u32 _port, Credit* _credit) {
  assert(false);
}

void RoutingAlgorithmTestRouter::sendCredit(u32 _port, u32 _vc) {
  assert(false);
}

void RoutingAlgorithmTestRouter::sendFlit(u32 _port, Flit* _flit) {
  assert(false);
}

f64 RoutingAlgorithmTestRouter::congestionStatus(u32 _port, u32 _vc) const {
  assert(false);
}
