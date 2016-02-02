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
#include "network/NetworkFactory.h"

#include <cassert>

#include "network/foldedclos/Network.h"
#include "network/hyperx/Network.h"
#include "network/torus/Network.h"
#include "network/uno/Network.h"

Network* NetworkFactory::createNetwork(
    const std::string& _name, const Component* _parent,
    Json::Value _settings) {
  std::string topology = _settings["topology"].asString();

  if (topology == "folded_clos") {
    return new FoldedClos::Network(_name, _parent, _settings);
  } else if (topology == "hyperx") {
    return new HyperX::Network(_name, _parent, _settings);
  } else if (topology == "torus") {
    return new Torus::Network(_name, _parent, _settings);
  } else if (topology == "uno") {
    return new Uno::Network(_name, _parent, _settings);
  } else {
    fprintf(stderr, "unknown network topology: %s\n", topology.c_str());
    assert(false);
  }
}
