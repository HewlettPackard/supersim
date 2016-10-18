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

Router::Router(const std::string& _name, const Component* _parent, u32 _id,
               const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
               MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent),
      PortedDevice(_id, _address, _numPorts, _numVcs),
      metadataHandler_(_metadataHandler) {}

Router::~Router() {}

void Router::packetArrival(Packet* _packet) const {
  metadataHandler_->packetArrival(_packet);
}
