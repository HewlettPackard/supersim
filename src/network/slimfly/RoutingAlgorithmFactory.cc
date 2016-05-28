/*
 * Copyright 2016 Ashish Chaudhari, Franky Romero, Nehal Bhandari, Wasam Altoyan
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

#include "network/slimfly/RoutingAlgorithmFactory.h"

#include <cassert>

#include "network/slimfly/MinRoutingAlgorithm.h"
#include "network/RoutingAlgorithm.h"

namespace SlimFly {

RoutingAlgorithmFactory::RoutingAlgorithmFactory(
    u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration, Json::Value _settings,
    DimensionalArray<RoutingTable*>* _routingTables,
    const std::vector<u32>& _X, const std::vector<u32>& _X_i
) : ::RoutingAlgorithmFactory(), numVcs_(_numVcs),
      dimensionWidths_(_dimensionWidths), concentration_(_concentration),
      settings_(_settings), routingTables_(_routingTables),
      X_(_X), X_i_(_X_i) {}

RoutingAlgorithmFactory::~RoutingAlgorithmFactory() {}

RoutingAlgorithm* RoutingAlgorithmFactory::createRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _inputPort) {
  std::string algorithm = settings_["algorithm"].asString();
  std::string impl = settings_["implementation"].asString();
  bool adaptive = settings_["adaptive"].asBool();
  u32 latency = settings_["latency"].asUInt();

  if (algorithm == "minimal") {
    return new SlimFly::MinRoutingAlgorithm(
        _name, _parent, _router, latency, numVcs_, dimensionWidths_,
        concentration_, routingTables_, X_, X_i_, impl, adaptive);
  } else {
    fprintf(stderr, "Unknown routing algorithm: '%s'\n", algorithm.c_str());
    assert(false);
    return nullptr;
  }
}

}  // namespace SlimFly
