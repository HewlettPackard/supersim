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
#include "router/RouterFactory.h"

#include <cassert>

#include "router/inputoutputqueued/Router.h"
#include "router/inputqueued/Router.h"

Router* RouterFactory::createRouter(
    const std::string& _name, const Component* _parent,
    RoutingFunctionFactory* _routingFunctionFactory,
    Json::Value _settings) {
  std::string architecture = _settings["architecture"].asString();

  if (architecture == "input_queued") {
    return new InputQueued::Router(
        _name, _parent, _routingFunctionFactory, _settings);
  } else if (architecture == "input_output_queued") {
    return new InputOutputQueued::Router(
        _name, _parent, _routingFunctionFactory, _settings);
  } else {
    fprintf(stderr, "unknown router architecture: %s\n", architecture.c_str());
    assert(false);
  }
}
