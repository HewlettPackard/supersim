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
#ifndef NETWORK_BUTTERFLY_INJECTIONALGORITHMFACTORY_H_
#define NETWORK_BUTTERFLY_INJECTIONALGORITHMFACTORY_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "interface/Interface.h"
#include "network/InjectionAlgorithmFactory.h"

namespace Butterfly {

class InjectionAlgorithmFactory : public ::InjectionAlgorithmFactory {
 public:
  InjectionAlgorithmFactory(u32 _numVcs, Json::Value _settings);
  ~InjectionAlgorithmFactory();
  InjectionAlgorithm* createInjectionAlgorithm(
      const std::string& _name, const Component* _parent,
      Interface* _interface);

 private:
  const u32 numVcs_;
  Json::Value settings_;
};

}  // namespace Butterfly

#endif  // NETWORK_BUTTERFLY_INJECTIONALGORITHMFACTORY_H_
