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
#ifndef NETWORK_ROUTINGFUNCTIONFACTORY_H_
#define NETWORK_ROUTINGFUNCTIONFACTORY_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <string>

class Component;
class Router;
class RoutingFunction;

class RoutingFunctionFactory {
 public:
  RoutingFunctionFactory();
  virtual ~RoutingFunctionFactory();
  virtual RoutingFunction* createRoutingFunction(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _inputPort, Json::Value _settings) = 0;
};

#endif  // NETWORK_ROUTINGFUNCTIONFACTORY_H_
