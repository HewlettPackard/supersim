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
#include "network/hyperx/ValiantsRoutingAlgorithm.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <vector>

#include "network/hyperx/RoutingAlgorithm.h"
#include "routing/RoutingAlgorithm_TEST.h"

TEST(HyperX_ValiantsRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 2, 3, 4});
  std::vector<std::string> outputTypes({"vc", "port"});
  std::vector<std::string> outputAlgs({"minimal", "random"});
  std::vector<u32> maxOutputs({0, 1, 2, 3, 4});
  std::vector<std::string> intNodes({
      "regular", "source", "dest", "source_dest", "unaligned", "minimal_vc",
          "minimal_port"});
  std::vector<std::string> minTypes({
      "dimension_order", "random", "adaptive"});
  std::vector<char> shortCuts({true, false});

  for (auto& latency : latencies) {
    for (auto& outputType : outputTypes) {
      for (auto& outputAlg : outputAlgs) {
        for (auto& maxOutput : maxOutputs) {
          for (auto& intNode : intNodes) {
            for (auto& minType : minTypes) {
              for (auto& shortCut : shortCuts) {
                Json::Value settings;
                settings["algorithm"] = "valiants";
                settings["latency"] = latency;
                settings["output_type"] = outputType;
                settings["output_algorithm"] = outputAlg;
                settings["max_outputs"] = maxOutput;
                settings["intermediate_node"] = intNode;
                settings["minimal"] = minType;
                settings["short_cut"] = static_cast<bool>(shortCut);
                RoutingAlgorithmTestRouter tr("Router", 16, 24);
                HyperX::RoutingAlgorithm* ra = HyperX::RoutingAlgorithm::create(
                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1, {2, 3, 4},
                    {4, 3, 1}, 6, settings);
                delete ra;
              }
            }
          }
        }
      }
    }
  }
}
