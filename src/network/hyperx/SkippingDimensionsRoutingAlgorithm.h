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
#ifndef NETWORK_HYPERX_SKIPPINGDIMENSIONSROUTINGALGORITHM_H_
#define NETWORK_HYPERX_SKIPPINGDIMENSIONSROUTINGALGORITHM_H_

#include <colhash/tuplehash.h>
#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "event/Component.h"
#include "network/hyperx/RoutingAlgorithm.h"
#include "network/hyperx/util.h"
#include "router/Router.h"

namespace HyperX {

class SkippingDimensionsRoutingAlgorithm : public RoutingAlgorithm {
 public:
  SkippingDimensionsRoutingAlgorithm(
      const std::string& _name, const Component* _parent, Router* _router,
      u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
      const std::vector<u32>& _dimensionWidths,
      const std::vector<u32>& _dimensionWeights,
      u32 _concentration, Json::Value _settings);
  ~SkippingDimensionsRoutingAlgorithm();

 protected:
  void processRequest(Flit* _flit,
                      RoutingAlgorithm::Response* _response) override;

 private:
  u32 maxOutputs_;
  OutputAlg outputAlg_;
  f64 thresholdMin_;
  f64 thresholdNonMin_;
  f64 threshold_;
  f64 iBias_;
  f64 cBias_;
  BiasScheme biasMode_;
  HopCountMode hopCountMode_;
  f64 step_;

  bool outputTypePort_;
  SkippingRoutingAlg skippingType_;
  SkippingRoutingAlg finishingType_;
  DecisionScheme decisionScheme_;
  u32 numVcSets_;
  // u32 finishVcs_;
  u32 numRounds_;

  std::unordered_set<std::tuple<u32, u32, f64>> outputVcs1_;
  std::unordered_set<std::tuple<u32, u32, f64>> outputVcs2_;
  std::unordered_set<std::tuple<u32, u32, f64>> outputVcs3_;
  std::unordered_set<std::tuple<u32, u32, f64>> vcPool_;
  std::unordered_set<std::tuple<u32, u32, f64>> outputPorts_;
};

}  // namespace HyperX

#endif  // NETWORK_HYPERX_SKIPPINGDIMENSIONSROUTINGALGORITHM_H_
