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
#include "network/foldedclos/RoutingAlgorithmFactory.h"

#include <cassert>

#include "network/foldedclos/McaRoutingAlgorithm.h"
#include "network/foldedclos/LcaRoutingAlgorithm.h"
#include "network/RoutingAlgorithm.h"

namespace FoldedClos {

RoutingAlgorithmFactory::RoutingAlgorithmFactory(
    u32 _numVcs, u32 _numPorts, u32 _numLevels, u32 _level,
    Json::Value _settings)
    : ::RoutingAlgorithmFactory(), numVcs_(_numVcs), numPorts_(_numPorts),
      numLevels_(_numLevels), level_(_level), settings_(_settings) {}

RoutingAlgorithmFactory::~RoutingAlgorithmFactory() {}

RoutingAlgorithm* RoutingAlgorithmFactory::createRoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _inputPort) {

  std::string algorithm = settings_["algorithm"].asString();
  assert(settings_.isMember("latency"));
  u32 latency = settings_["latency"].asUInt();
  assert(settings_.isMember("adaptive"));
  bool adaptive = settings_["adaptive"].asBool();

  if (algorithm == "most_common_ancestor") {
    return new McaRoutingAlgorithm(_name, _parent, _router, latency,
                                   numVcs_, numPorts_, numLevels_, level_,
                                   _inputPort, adaptive);
  } else if (algorithm == "least_common_ancestor") {
    return new LcaRoutingAlgorithm(_name, _parent, _router, latency,
                                   numVcs_, numPorts_, numLevels_, level_,
                                   _inputPort, adaptive);
  } else {
    fprintf(stderr, "Unknown routing algorithm: '%s'\n", algorithm.c_str());
    assert(false);
  }
}

}  // namespace FoldedClos
