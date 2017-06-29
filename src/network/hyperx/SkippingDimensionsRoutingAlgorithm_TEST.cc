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
#include "network/hyperx/SkippingDimensionsRoutingAlgorithm.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <vector>

#include "network/hyperx/RoutingAlgorithm.h"
#include "routing/RoutingAlgorithm_TEST.h"

TEST(HyperX_SkippingDimensionsRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 2, 3, 4});
  std::vector<std::string> outputTypes({"vc", "port"});
  std::vector<std::string> outputAlgs({"minimal", "random"});
  std::vector<u32> maxOutputs({0, 1, 4});

  std::vector<std::string> decSchemes({
      "monolithic_weighted", "staged_threshold", "threshold_weighted"});
  std::vector<std::string> hopCountModes({"absolute", "normalized"});

  std::vector<std::string> skipAlgs({"dimension_adaptive", "dimension_order"});
  std::vector<std::string> finiAlgs({"dimension_adaptive", "dimension_order"});

  std::vector<u32> skipRounds({1, 3});
  std::vector<f64> thresholds({0.0, 0.5, 1.0});

  std::vector<f64> thresholdMin({0.0, 0.5, 1.0});
  std::vector<f64> thresholdNonmin({0.0, 0.5, 1.0});

  std::vector<f64> cbiass({0.0, 0.3, 1.0});
  std::vector<f64> ibiass({0.0, 0.3, 1.0});

  std::vector<f64> steps({0.0, 0.7, 1.0});

  for (auto& hcm : hopCountModes) {
    for (auto& skipAlg : skipAlgs) {
      for (auto& finiAlg : skipAlgs) {
        for (auto& latency : latencies) {
          for (auto& outputType : outputTypes) {
            for (auto& outputAlg : outputAlgs) {
              for (auto& maxOutput : maxOutputs) {
                for (auto& skipRound : skipRounds) {
                  for (auto& step : steps) {
                    // start custom
                    for (auto& decScheme : decSchemes) {
                      if (decScheme == "monolithic_weighted") {
                        for (auto& ibias : ibiass) {
                          for (auto& cbias : cbiass) {
                            Json::Value settings;
                            settings["algorithm"] = "skipping_dimensions";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["hop_count_mode"] = hcm;
                            settings["independent_bias"] = ibias;
                            settings["decision_scheme"] = decScheme;
                            settings["congestion_bias"] = cbias;
                            settings["skipping_algorithm"] = skipAlg;
                            settings["finishing_algorithm"] = finiAlg;
                            settings["num_rounds"] = skipRound;
                            settings["step"] = step;
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                    {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      } else if (decScheme == "staged_threshold") {
                        for (auto& ithresholdMin : thresholdMin) {
                          for (auto& ithresholdNonmin : thresholdNonmin) {
                            Json::Value settings;
                            settings["algorithm"] = "skipping_dimensions";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["hop_count_mode"] = hcm;
                            settings["decision_scheme"] = decScheme;
                            settings["skipping_algorithm"] = skipAlg;
                            settings["finishing_algorithm"] = finiAlg;
                            settings["threshold_min"] = ithresholdMin;
                            settings["threshold_nonmin"] =
                                ithresholdNonmin;
                            settings["num_rounds"] = skipRound;
                            settings["step"] = step;
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                    {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      } else if (decScheme == "threshold_weighted") {
                        for (auto& threshold : thresholds) {
                          Json::Value settings;
                          settings["algorithm"] = "skipping_dimensions";
                          settings["latency"] = latency;
                          settings["output_type"] = outputType;
                          settings["output_algorithm"] = outputAlg;
                          settings["max_outputs"] = maxOutput;
                          settings["hop_count_mode"] = hcm;
                          settings["decision_scheme"] = decScheme;
                          settings["skipping_algorithm"] = skipAlg;
                          settings["finishing_algorithm"] = finiAlg;
                          settings["threshold"] = threshold;
                          settings["num_rounds"] = skipRound;
                          settings["step"] = step;
                          RoutingAlgorithmTestRouter tr("Router", 16, 24);
                          HyperX::RoutingAlgorithm* ra =
                              HyperX::RoutingAlgorithm::create(
                                  "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                  {2, 3, 4}, {4, 3, 1}, 6, settings);
                          delete ra;
                        }
                      } else {
                        ASSERT_TRUE(false);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
