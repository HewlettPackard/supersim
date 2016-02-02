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
#ifndef NETWORK_HYPERX_ROUTINGFUNCTIONFACTORY_H_
#define NETWORK_HYPERX_ROUTINGFUNCTIONFACTORY_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingFunctionFactory.h"

namespace HyperX {

class RoutingFunctionFactory : public ::RoutingFunctionFactory {
 public:
  RoutingFunctionFactory(u32 _numVcs, const std::vector<u32>& _dimensionWidths,
                         const std::vector<u32>& _dimensionWeights,
                         u32 _concentration);
  ~RoutingFunctionFactory();
  RoutingFunction* createRoutingFunction(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 inputPort, Json::Value _settings);

 private:
  u32 numVcs_;
  const std::vector<u32> dimensionWidths_;
  const std::vector<u32> dimensionWeights_;
  u32 concentration_;
};

}  // namespace HyperX

#endif  // NETWORK_HYPERX_ROUTINGFUNCTIONFACTORY_H_
