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
#ifndef NETWORK_FOLDEDCLOS_ROUTINGFUNCTIONFACTORY_H_
#define NETWORK_FOLDEDCLOS_ROUTINGFUNCTIONFACTORY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "network/RoutingFunctionFactory.h"

namespace FoldedClos {

class RoutingFunctionFactory : public ::RoutingFunctionFactory {
 public:
  RoutingFunctionFactory(u32 _numVcs, u32 _numPorts, u32 _numLevels, u32 level);
  ~RoutingFunctionFactory();
  RoutingFunction* createRoutingFunction(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 inputPort, Json::Value _settings);

 private:
  u32 numVcs_;
  u32 numPorts_;
  u32 numLevels_;
  u32 level_;
};

}  // namespace FoldedClos

#endif  // NETWORK_FOLDEDCLOS_ROUTINGFUNCTIONFACTORY_H_
