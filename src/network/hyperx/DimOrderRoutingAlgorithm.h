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
#ifndef NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_
#define NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_

#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"

namespace HyperX {

class DimOrderRoutingAlgorithm : public RoutingAlgorithm {
 public:
  DimOrderRoutingAlgorithm(const std::string& _name, const Component* _parent,
                          Router* _router, u64 _latency,
                          const std::vector<u32>& _dimensionWidths,
                          const std::vector<u32>& _dimensionWeights,
                          u32 _concentration);
  ~DimOrderRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  const u32 numVcs_;
  const std::vector<u32> dimensionWidths_;
  const std::vector<u32> dimensionWeights_;
  const u32 concentration_;
};

}  // namespace HyperX

#endif  // NETWORK_HYPERX_DIMORDERROUTINGALGORITHM_H_
