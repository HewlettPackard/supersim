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
#include "network/hyperx/DalRoutingAlgorithm.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <vector>

#include "network/hyperx/RoutingAlgorithm.h"
#include "routing/RoutingAlgorithm_TEST.h"

TEST(HyperX_DalRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 2, 3, 4});
  std::vector<std::string> outputTypes({"vc", "port"});
  std::vector<std::string> outputAlgs({"minimal", "random"});
  std::vector<u32> maxOutputs({0, 1, 2, 3, 4});

  std::vector<std::string> adTypes({
      "dimension_adaptive", "dimension_order", "variable"});
  std::vector<std::string> decSchemes({
      "monolithic_weighted", "staged_threshold", "threshold_weighted"});
  std::vector<u32> maxDeroutes({0, 1, 2, 3, 4});
  std::vector<std::string> hopCountModes({"absolute", "normalized"});

  std::vector<f64> ibiass({0.0, 0.3, 1.0});
  std::vector<f64> cbiass({0.0, 0.3, 1.0});
  std::vector<f64> thresholdMin({0.0, 0.5, 1.0});
  std::vector<f64> thresholdNonmin({0.0, 0.5, 1.0});
  std::vector<f64> thresholds({0.0, 0.5, 1.0});
  std::vector<char> multiDeroutes({true, false});

  for (auto& ibias : ibiass) {
    for (auto& latency : latencies) {
      for (auto& outputType : outputTypes) {
        for (auto& outputAlg : outputAlgs) {
          for (auto& maxOutput : maxOutputs) {
            // start custom
            for (auto& decScheme : decSchemes) {
              if (decScheme == "monolithic_weighted") {
                for (auto& hcm : hopCountModes) {
                  for (auto& cbias : cbiass) {
                    for (auto& adType : adTypes) {
                      if (adType == "variable") {
                        for (auto& maxDeroute : maxDeroutes) {
                          for (auto& multiDeroute : multiDeroutes) {
                            Json::Value settings;
                            settings["algorithm"] = "dal";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["independent_bias"] = ibias;
                            settings["decision_scheme"] = decScheme;
                            settings["hop_count_mode"] = hcm;
                            settings["congestion_bias"] = cbias;
                            settings["adaptivity_type"] = adType;
                            settings["max_deroutes"] = maxDeroute;
                            settings["multi_deroute"] =
                                static_cast<bool>(multiDeroute);
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                    {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      } else {  //  not "variable
                        Json::Value settings;
                        settings["algorithm"] = "dal";
                        settings["latency"] = latency;
                        settings["output_type"] = outputType;
                        settings["output_algorithm"] = outputAlg;
                        settings["max_outputs"] = maxOutput;
                        settings["independent_bias"] = ibias;
                        settings["decision_scheme"] = decScheme;
                        settings["hop_count_mode"] = hcm;
                        settings["congestion_bias"] = cbias;
                        settings["adaptivity_type"] = adType;
                        RoutingAlgorithmTestRouter tr("Router", 16, 24);
                        HyperX::RoutingAlgorithm* ra =
                            HyperX::RoutingAlgorithm::create(
                                "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                {2, 3, 4}, {4, 3, 1}, 6, settings);
                        delete ra;
                      }
                    }
                  }
                }
              } else if (decScheme == "staged_threshold") {
                for (auto& ithresholdMin : thresholdMin) {
                  for (auto& ithresholdNonmin : thresholdNonmin) {
                    for (auto& adType : adTypes) {
                      if (adType == "variable") {
                        for (auto& maxDeroute : maxDeroutes) {
                          for (auto& multiDeroute : multiDeroutes) {
                            Json::Value settings;
                            settings["algorithm"] = "dal";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["independent_bias"] = ibias;
                            settings["decision_scheme"] = decScheme;
                            settings["threshold_min"] = ithresholdMin;
                            settings["threshold_nonmin"] = ithresholdNonmin;
                            settings["adaptivity_type"] = adType;
                            settings["max_deroutes"] = maxDeroute;
                            settings["multi_deroute"] =
                                static_cast<bool>(multiDeroute);
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                  {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      } else {  //  not "variable
                        for (auto& ithresholdMin : thresholdMin) {
                          for (auto& ithresholdNonmin : thresholdNonmin) {
                            Json::Value settings;
                            settings["algorithm"] = "dal";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["independent_bias"] = ibias;
                            settings["decision_scheme"] = decScheme;
                            settings["threshold_min"] = ithresholdMin;
                            settings["threshold_nonmin"] = ithresholdNonmin;
                            settings["adaptivity_type"] = adType;
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                  {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      }
                    }
                  }
                }
              } else if (decScheme == "threshold_weighted") {
                for (auto& hcm : hopCountModes) {
                  for (auto& threshold : thresholds) {
                    for (auto& adType : adTypes) {
                      if (adType == "variable") {
                        for (auto& maxDeroute : maxDeroutes) {
                          for (auto& multiDeroute : multiDeroutes) {
                            Json::Value settings;
                            settings["algorithm"] = "dal";
                            settings["latency"] = latency;
                            settings["output_type"] = outputType;
                            settings["output_algorithm"] = outputAlg;
                            settings["max_outputs"] = maxOutput;
                            settings["independent_bias"] = ibias;
                            settings["decision_scheme"] = decScheme;
                            settings["hop_count_mode"] = hcm;
                            settings["threshold"] = threshold;
                            settings["adaptivity_type"] = adType;
                            settings["max_deroutes"] = maxDeroute;
                            settings["multi_deroute"] =
                                static_cast<bool>(multiDeroute);
                            RoutingAlgorithmTestRouter tr("Router", 16, 24);
                            HyperX::RoutingAlgorithm* ra =
                                HyperX::RoutingAlgorithm::create(
                                    "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                    {2, 3, 4}, {4, 3, 1}, 6, settings);
                            delete ra;
                          }
                        }
                      } else {  //  not "variable
                        Json::Value settings;
                        settings["algorithm"] = "dal";
                        settings["latency"] = latency;
                        settings["output_type"] = outputType;
                        settings["output_algorithm"] = outputAlg;
                        settings["max_outputs"] = maxOutput;
                        settings["independent_bias"] = ibias;
                        settings["decision_scheme"] = decScheme;
                        settings["hop_count_mode"] = hcm;
                        settings["threshold"] = threshold;
                        settings["adaptivity_type"] = adType;
                        RoutingAlgorithmTestRouter tr("Router", 16, 24);
                        HyperX::RoutingAlgorithm* ra =
                            HyperX::RoutingAlgorithm::create(
                                "RoutingAlgorithm", &tr, &tr, 1, 12, 1, 1,
                                {2, 3, 4}, {4, 3, 1}, 6, settings);
                        delete ra;
                      }
                    }
                  }
                }
              } else {
                ASSERT_TRUE(false);
              }
            }  // end custom
          }
        }
      }
    }
  }
}
