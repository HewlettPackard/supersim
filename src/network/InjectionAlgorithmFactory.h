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
#ifndef NETWORK_INJECTIONALGORITHMFACTORY_H_
#define NETWORK_INJECTIONALGORITHMFACTORY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

class Component;
class Interface;
class Router;
class InjectionAlgorithm;

class InjectionAlgorithmFactory {
 public:
  InjectionAlgorithmFactory();
  virtual ~InjectionAlgorithmFactory();
  virtual InjectionAlgorithm* createInjectionAlgorithm(
      const std::string& _name, const Component* _parent, Interface* _interface,
      Json::Value _settings) = 0;
};

#endif  // NETWORK_INJECTIONALGORITHMFACTORY_H_
