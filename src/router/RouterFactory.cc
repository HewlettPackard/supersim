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

#include <cassert>

#include "router/RouterFactory.h"
#include "router/Router.h"
#include "factory/AbstractFactory.h"



Router* RouterFactory::createRouter(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address,
    u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler,
    std::vector<RoutingAlgorithmFactory*>* _routingAlgorithmFactories,
    Json::Value _settings) {
  std::string architecture = _settings["architecture"].asString();

  Router *retval = AbstractFactory<Router,
      const std::string& , const Component*, u32,
      const std::vector<u32>&, u32, u32,
      MetadataHandler*,
      std::vector<RoutingAlgorithmFactory*>*,
      Json::Value
      >::Create(architecture.c_str(),
        _name, _parent, _id, _address, _numPorts, _numVcs, _metadataHandler,
        _routingAlgorithmFactories, _settings);
  if (!retval) {
    fprintf(stderr, "unknown router architecture: %s\n", architecture.c_str());
    assert(false);
  }
  return retval;
}
