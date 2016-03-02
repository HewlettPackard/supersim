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
#include "network/foldedclos/RoutingFunctionFactory.h"

#include <cassert>

#include "network/foldedclos/McaRoutingFunction.h"
#include "network/foldedclos/LcaRoutingFunction.h"
#include "network/RoutingFunction.h"

namespace FoldedClos {

RoutingFunctionFactory::RoutingFunctionFactory(
    u32 _numLevels, u32 _level)
    : ::RoutingFunctionFactory(), numLevels_(_numLevels), level_(_level) {}

RoutingFunctionFactory::~RoutingFunctionFactory() {}

RoutingFunction* RoutingFunctionFactory::createRoutingFunction(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _inputPort, Json::Value _settings) {

  std::string algorithm = _settings["algorithm"].asString();
  u32 latency = _settings["latency"].asUInt();

  if (algorithm == "most_common_ancestor") {
    return new McaRoutingFunction(_name, _parent, _router, latency,
                                  numLevels_, level_, _inputPort);
  } else if (algorithm == "least_common_ancestor") {
    return new LcaRoutingFunction(_name, _parent, _router, latency,
                                  numLevels_, level_, _inputPort);
  } else {
    fprintf(stderr, "Unknown routing algorithm: '%s'\n", algorithm.c_str());
    assert(false);
  }
}

}  // namespace FoldedClos
