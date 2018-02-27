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
#ifndef NETWORK_DRAGONFLY_VALIANTSROUTINGALGORITHM_H_
#define NETWORK_DRAGONFLY_VALIANTSROUTINGALGORITHM_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "network/dragonfly/RoutingAlgorithm.h"
#include "router/Router.h"
#include "routing/mode.h"
#include "routing/Reduction.h"

namespace Dragonfly {

class ValiantsRoutingAlgorithm : public RoutingAlgorithm {
 public:
  ValiantsRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
      u32 _localWidth, u32 _localWeight,
      u32 _globalWidth, u32 _globalWeight,
      u32 _concentration, u32 _routerRadix, u32 _globalPortsPerRouter,
      Json::Value _settings);
  ~ValiantsRoutingAlgorithm();

 protected:
  void processRequest(
      Flit* _flit, RoutingAlgorithm::Response* _response) override;

 private:
  void addPort(u32 _port, u32 _hops, u32 _routingClass);
  void addPortsToLocalRouter(u32 _src, u32 _dst, u32 _routingCLass);
  void routeToRemoteGroup(u32 _globalOffset, u32 _stage, bool _newStage,
                          u32 _thisRouter, u32 _directRc, u32 _indirectRc);

  bool smartIntermediateNode_;
  u32 rcs_;
  u32 localPortBase_;
  u32 globalPortBase_;

  const RoutingMode mode_;
  Reduction* reduction_;
};

}  // namespace Dragonfly

#endif  // NETWORK_DRAGONFLY_VALIANTSROUTINGALGORITHM_H_
