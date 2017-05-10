/*
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * See the NOTICE file distributed with this work for additional information
 * regarding copyright ownership. You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_
#define NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/hyperx/RoutingAlgorithm.h"
#include "router/Router.h"

namespace HyperX {

class DimOrderRoutingAlgorithm : public RoutingAlgorithm {
 public:
  DimOrderRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _baseVc, u32 _numVcs, const std::vector<u32>& _dimensionWidths,
      const std::vector<u32>& _dimensionWeights, u32 _concentration,
      u32 _inputPort, Json::Value _settings);
  ~DimOrderRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;
};

}  // namespace HyperX

#endif  // NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_
