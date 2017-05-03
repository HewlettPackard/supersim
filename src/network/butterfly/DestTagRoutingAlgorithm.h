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
#ifndef NETWORK_BUTTERFLY_DESTTAGROUTINGALGORITHM_H_
#define NETWORK_BUTTERFLY_DESTTAGROUTINGALGORITHM_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/butterfly/RoutingAlgorithm.h"
#include "router/Router.h"

namespace Butterfly {

class DestTagRoutingAlgorithm : public RoutingAlgorithm {
 public:
  DestTagRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _baseVc, u32 _numVcs, u32 _numPorts, u32 _numStages,
      u32 _stage, Json::Value _settings);
  ~DestTagRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;
};

}  // namespace Butterfly

#endif  // NETWORK_BUTTERFLY_DESTTAGROUTINGALGORITHM_H_
