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
#ifndef NETWORK_SLIMFLY_MINROUTINGALGORITHM_H_
#define NETWORK_SLIMFLY_MINROUTINGALGORITHM_H_

#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "router/Router.h"
#include "RoutingTable.h"

namespace SlimFly {

class MinRoutingAlgorithm : public RoutingAlgorithm {
 public:
  MinRoutingAlgorithm(const std::string& _name, const Component* _parent, 
                Router* _router, u64 _latency, u32 _numVcs, 
                const std::vector<u32>& _dimensionWidths, 
                u32 _concentration, 
                const std::vector<u32>& _X, const std::vector<u32>& _X_i);
  ~MinRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  const u32 numVcs_;
  const std::vector<u32> dimensionWidths_;
  const u32 concentration_;
  const std::vector<u32>& X_, X_i_;

  bool checkConnected(
    u32 graph, u32 srcRow, u32 dstRow);
  bool checkConnectedAcross(
    const std::vector<u32>& routerAddress, const std::vector<u32>* destinationAddress);
};

}  // namespace SlimFly

#endif  // NETWORK_SLIMFLY_MINROUTINGALGORITHM_H_
