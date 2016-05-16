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

#include "network/slimfly/DimOrderRoutingAlgorithm.h"
#include "network/RoutingAlgorithm.h"

namespace SlimFly {

RoutingAlgorithmFactory::RoutingAlgorithmFactory(
    u32 _numVcs, const std::vector<u32>& _dimensionWidths,
    u32 _concentration, Json::Value _settings)
    : ::RoutingAlgorithmFactory(), numVcs_(_numVcs),
      dimensionWidths_(_dimensionWidths), concentration_(_concentration), 
			settings_(_settings) {}

RoutingAlgorithmFactory::~RoutingAlgorithmFactory() {}

RoutingAlgorithm* RoutingAlgorithmFactory::createRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _inputPort) {
  std::string algorithm = settings_["algorithm"].asString();
  u32 latency = settings_["latency"].asUInt();

  if (algorithm == "dimension_order") {
    return new SlimFly::DimOrderRoutingAlgorithm(
        _name, _parent, _router, latency, numVcs_, dimensionWidths_,
        concentration_);
  } else {
   fprintf(stderr, "Unknown routing algorithm: '%s'\n", algorithm.c_str());
   assert(false);
  }
}

}  // namespace SlimFly
