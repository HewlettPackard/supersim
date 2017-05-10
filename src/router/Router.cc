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
#include "router/Router.h"

#include <factory/Factory.h>

#include <cassert>

Router::Router(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent),
      PortedDevice(_id, _address, _numPorts, _numVcs),
      network_(_network), metadataHandler_(_metadataHandler) {}

Router::~Router() {}

Router* Router::create(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings) {
  // retrieve the architecture
  const std::string& architecture = _settings["architecture"].asString();

  // attempt to build the router
  Router* router = factory::Factory<Router, ROUTER_ARGS>::create(
      architecture, _name, _parent, _network, _id, _address, _numPorts, _numVcs,
      _metadataHandler, _settings);

  // check that the factory had this architecture
  if (router == nullptr) {
    fprintf(stderr, "unknown router architecture: %s\n", architecture.c_str());
    assert(false);
  }
  return router;
}

void Router::packetArrival(Packet* _packet) const {
  metadataHandler_->packetArrival(_packet);
}
