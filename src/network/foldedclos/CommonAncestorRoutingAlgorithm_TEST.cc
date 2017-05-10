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
#include "network/foldedclos/CommonAncestorRoutingAlgorithm.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "network/foldedclos/RoutingAlgorithm.h"
#include "network/RoutingAlgorithm_TEST.h"

TEST(FoldedClos_CommonAncestorRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 2, 3});

  std::vector<char> leasts({true, false});
  std::vector<std::string> modes({"all", "port", "vc"});
  std::vector<char> adaptives({true, false});

  for (auto& latency : latencies) {
    for (auto& least : leasts) {
      for (auto& mode : modes) {
        for (auto& adaptive : adaptives) {
          Json::Value settings;
          settings["algorithm"] = "common_ancestor";
          settings["latency"] = latency;
          settings["least_common_ancestor"] = least;
          settings["mode"] = mode;
          settings["adaptive"] = adaptive;
          RoutingAlgorithmTestRouter tr("Router", 64, 16);
          FoldedClos::RoutingAlgorithm* ra =
              FoldedClos::RoutingAlgorithm::create(
                  "RoutingAlgorithm", &tr, &tr, 1, 13, 64, 3, 9, settings);
          delete ra;
        }
      }
    }
  }
}
