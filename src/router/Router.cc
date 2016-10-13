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
#include "router/Router.h"

#include <cassert>

Router::Router(const std::string& _name, const Component* _parent,
               u32 _numPorts, u32 _numVcs, const std::vector<u32>& _address,
               MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent), numPorts_(_numPorts), numVcs_(_numVcs),
      address_(_address), metadataHandler_(_metadataHandler) {}

Router::~Router() {}

u32 Router::numPorts() const {
  return numPorts_;
}

u32 Router::numVcs() const {
  return numVcs_;
}

const std::vector<u32>& Router::getAddress() const {
  return address_;
}

u32 Router::vcIndex(u32 _port, u32 _vc) const {
  return (_port * numVcs_) + _vc;
}

void Router::vcIndexInv(u32 _vcIdx, u32* _port, u32* _vc) const {
  assert(_vcIdx < (numPorts_ * numVcs_));
  *_port = _vcIdx / numVcs_;
  *_vc = _vcIdx % numVcs_;
}

void Router::packetArrival(Packet* _packet) const {
  metadataHandler_->packetArrival(_packet);
}

f64 Router::congestionStatus(u32 _port, u32 _vc) const {
  assert(false);  // subclasses should override this if it needs to be supported
}
