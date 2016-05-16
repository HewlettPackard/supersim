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

#ifndef NETWORK_SLIMFLY_ROUTINGALGORITHMFACTORY_H_
#define NETWORK_SLIMFLY_ROUTINGALGORITHMFACTORY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithmFactory.h"

namespace SlimFly {

class RoutingAlgorithmFactory : public ::RoutingAlgorithmFactory {
 public:
  RoutingAlgorithmFactory(u32 _numVcs, const std::vector<u32>& _dimensionWidths,
                          u32 _concentration, Json::Value _settings);
  ~RoutingAlgorithmFactory();
  RoutingAlgorithm* createRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 inputPort);

 private:
  const u32 numVcs_;
  const std::vector<u32> dimensionWidths_;
  const std::vector<u32> dimensionWeights_;
  const u32 concentration_;
  Json::Value settings_;
};

}  // namespace SlimFly

#endif  // NETWORK_SLIMFLY_ROUTINGALGORITHMFACTORY_H_
