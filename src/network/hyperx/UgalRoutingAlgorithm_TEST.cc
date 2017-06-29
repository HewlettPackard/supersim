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
#include "network/hyperx/UgalRoutingAlgorithm.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <vector>

#include "network/hyperx/RoutingAlgorithm.h"
#include "routing/RoutingAlgorithm_TEST.h"

TEST(HyperX_UgalRoutingAlgorithm, construct) {
  std::vector<u32> latencies({1, 4});
  std::vector<std::string> outputTypes({"vc", "port"});
  std::vector<std::string> outputAlgs({"minimal", "random"});
  std::vector<std::string> hopCountModes({"absolute", "normalized"});
  std::vector<u32> maxOutputs({0, 1, 4});
  std::vector<std::string> intNodes({
      "regular", "source", "dest", "source_dest", "unaligned", "minimal_vc",
          "minimal_port"});
  std::vector<std::string> minTypes({
      "dimension_order", "random", "adaptive"});
  std::vector<std::string> nonMinTypes({"valiants", "least_congested_queue"});
  std::vector<char> shortCuts({true, false});
  std::vector<char> minAllVcSets({true, false});

  std::vector<std::string> decSchemes({
      "monolithic_weighted", "staged_threshold", "threshold_weighted"});
  // monolithic weighted only
  std::vector<f64> ibiass({0.0, 0.4});
  std::vector<f64> cbiass({0.0, 0.6});
  std::vector<std::string> biasModes({"regular", "bimodal", "proportional",
          "differential", "proportional_differential"});
  // threshold only
  std::vector<f64> thresholdMin({0.0, 0.5, 1.0});
  std::vector<f64> thresholdNonmin({0.0, 0.5, 1.0});
  // threshold weighted
  std::vector<f64> thresholds({0.0, 0.5, 1.0});

  for (auto& nonMinType : nonMinTypes) {
    for (auto& minAllVcSet : minAllVcSets) {
      if (nonMinType == "valiants") {
        for (auto& latency : latencies) {
          for (auto& outputType : outputTypes) {
            for (auto& outputAlg : outputAlgs) {
              for (auto& maxOutput : maxOutputs) {
                for (auto& shortCut : shortCuts) {
                  for (auto& ibias : ibiass) {
                    for (auto& minType : minTypes) {
                      for (auto& intNode : intNodes) {
                        for (auto& decScheme : decSchemes) {
                          if (decScheme == "monolithic_weighted") {
                            for (auto& cbias : cbiass) {
                              for (auto& biasMode : biasModes) {
                                for (auto& hopCountMode : hopCountModes) {
                                  Json::Value settings;
                                  settings["algorithm"] = "ugal";
                                  settings["latency"] = latency;
                                  settings["output_type"] = outputType;
                                  settings["output_algorithm"] = outputAlg;
                                  settings["hop_count_mode"] = hopCountMode;
                                  settings["max_outputs"] = maxOutput;
                                  settings["minimal"] = minType;
                                  settings["non_minimal"] = nonMinType;
                                  settings["intermediate_node"] = intNode;
                                  settings["min_all_vc_sets"] =
                                      static_cast<bool>(minAllVcSet);
                                  settings["short_cut"] =
                                      static_cast<bool>(shortCut);
                                  settings["independent_bias"] = ibias;
                                  settings["decision_scheme"] = decScheme;
                                  settings["congestion_bias"] = cbias;
                                  settings["bias_mode"] = biasMode;
                                  RoutingAlgorithmTestRouter tr
                                      ("Router", 16, 24);
                                  HyperX::RoutingAlgorithm* ra =
                                      HyperX::RoutingAlgorithm::create(
                                          "RoutingAlgorithm", &tr, &tr, 1, 12,
                                          1, 1, {2, 3, 4}, {4, 3, 1}, 6,
                                          settings);
                                  delete ra;
                                }
                              }
                            }
                          } else if (decScheme == "staged_threshold") {
                            for (auto& ithresholdMin : thresholdMin) {
                              for (auto& ithresholdNonmin : thresholdNonmin) {
                                Json::Value settings;
                                settings["algorithm"] = "ugal";
                                settings["latency"] = latency;
                                settings["output_type"] = outputType;
                                settings["output_algorithm"] = outputAlg;
                                settings["max_outputs"] = maxOutput;
                                settings["minimal"] = minType;
                                settings["non_minimal"] = nonMinType;
                                settings["intermediate_node"] = intNode;
                                settings["min_all_vc_sets"] =
                                    static_cast<bool>(minAllVcSet);
                                settings["short_cut"] =
                                    static_cast<bool>(shortCut);
                                settings["independent_bias"] = ibias;
                                settings["decision_scheme"] = decScheme;
                                settings["threshold_min"] = ithresholdMin;
                                settings["threshold_nonmin"] =
                                    ithresholdNonmin;
                                RoutingAlgorithmTestRouter tr(
                                    "Router", 16, 24);
                                HyperX::RoutingAlgorithm* ra =
                                    HyperX::RoutingAlgorithm::create(
                                        "RoutingAlgorithm", &tr, &tr, 1, 12,
                                        1, 1,
                                        {2, 3, 4}, {4, 3, 1}, 6, settings);
                                delete ra;
                              }
                            }
                          } else if (decScheme == "threshold_weighted") {
                            for (auto& threshold : thresholds) {
                              for (auto& hopCountMode : hopCountModes) {
                                Json::Value settings;
                                settings["algorithm"] = "ugal";
                                settings["latency"] = latency;
                                settings["output_type"] = outputType;
                                settings["output_algorithm"] = outputAlg;
                                settings["hop_count_mode"] = hopCountMode;
                                settings["max_outputs"] = maxOutput;
                                settings["minimal"] = minType;
                                settings["non_minimal"] = nonMinType;
                                settings["intermediate_node"] = intNode;
                                settings["min_all_vc_sets"] =
                                    static_cast<bool>(minAllVcSet);
                                settings["short_cut"] =
                                    static_cast<bool>(shortCut);
                                settings["independent_bias"] = ibias;
                                settings["decision_scheme"] = decScheme;
                                settings["threshold"] = threshold;
                                RoutingAlgorithmTestRouter tr(
                                    "Router", 16, 24);
                                HyperX::RoutingAlgorithm* ra =
                                    HyperX::RoutingAlgorithm::create(
                                        "RoutingAlgorithm", &tr, &tr, 1, 12,
                                        1, 1,
                                        {2, 3, 4}, {4, 3, 1}, 6, settings);
                                delete ra;
                              }
                            }
                          } else if (decScheme == "threshold_minimal") {
                            for (auto& threshold : thresholds) {
                              Json::Value settings;
                              settings["algorithm"] = "ugal";
                              settings["latency"] = latency;
                              settings["output_type"] = outputType;
                              settings["output_algorithm"] = outputAlg;
                              settings["max_outputs"] = maxOutput;
                              settings["minimal"] = minType;
                              settings["non_minimal"] = nonMinType;
                              settings["intermediate_node"] = intNode;
                              settings["min_all_vc_sets"] =
                                  static_cast<bool>(minAllVcSet);
                              settings["short_cut"] =
                                  static_cast<bool>(shortCut);
                              settings["independent_bias"] = ibias;
                              settings["decision_scheme"] = decScheme;
                              settings["threshold"] = threshold;
                              RoutingAlgorithmTestRouter tr(
                                  "Router", 16, 24);
                              HyperX::RoutingAlgorithm* ra =
                                  HyperX::RoutingAlgorithm::create(
                                      "RoutingAlgorithm", &tr, &tr, 1, 12,
                                      1, 1,
                                      {2, 3, 4}, {4, 3, 1}, 6, settings);
                              delete ra;
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
      } else {  // not "valiants" = "least_congested_queue"
        for (auto& latency : latencies) {
          for (auto& outputType : outputTypes) {
            for (auto& outputAlg : outputAlgs) {
              for (auto& maxOutput : maxOutputs) {
                for (auto& shortCut : shortCuts) {
                  for (auto& ibias : ibiass) {
                    for (auto& minType : minTypes) {
                      for (auto& decScheme : decSchemes) {
                        if (decScheme == "monolithic_weighted") {
                          for (auto& cbias : cbiass) {
                            for (auto& biasMode : biasModes) {
                              for (auto& hopCountMode : hopCountModes) {
                                Json::Value settings;
                                settings["algorithm"] = "ugal";
                                settings["latency"] = latency;
                                settings["output_type"] = outputType;
                                settings["output_algorithm"] = outputAlg;
                                settings["hop_count_mode"] = hopCountMode;
                                settings["max_outputs"] = maxOutput;
                                settings["minimal"] = minType;
                                settings["non_minimal"] = nonMinType;
                                settings["min_all_vc_sets"] =
                                    static_cast<bool>(minAllVcSet);
                                settings["short_cut"] =
                                    static_cast<bool>(shortCut);
                                settings["independent_bias"] = ibias;
                                settings["decision_scheme"] = decScheme;
                                settings["congestion_bias"] = cbias;
                                settings["bias_mode"] = biasMode;
                                RoutingAlgorithmTestRouter tr("Router",
                                                              16, 24);
                                HyperX::RoutingAlgorithm* ra =
                                    HyperX::RoutingAlgorithm::create(
                                        "RoutingAlgorithm", &tr, &tr, 1, 12,
                                        1, 1, {2, 3, 4}, {4, 3, 1},
                                        6, settings);
                                delete ra;
                              }
                            }
                          }
                        } else if (decScheme == "staged_threshold") {
                          for (auto& ithresholdMin : thresholdMin) {
                            for (auto& ithresholdNonmin : thresholdNonmin) {
                              Json::Value settings;
                              settings["algorithm"] = "ugal";
                              settings["latency"] = latency;
                              settings["output_type"] = outputType;
                              settings["output_algorithm"] = outputAlg;
                              settings["max_outputs"] = maxOutput;
                              settings["minimal"] = minType;
                              settings["non_minimal"] = nonMinType;
                              settings["min_all_vc_sets"] =
                                  static_cast<bool>(minAllVcSet);
                              settings["short_cut"] =
                                  static_cast<bool>(shortCut);
                              settings["independent_bias"] = ibias;
                              settings["decision_scheme"] = decScheme;
                              settings["threshold_min"] = ithresholdMin;
                              settings["threshold_nonmin"] =
                                  ithresholdNonmin;
                              RoutingAlgorithmTestRouter tr(
                                  "Router", 16, 24);
                              HyperX::RoutingAlgorithm* ra =
                                  HyperX::RoutingAlgorithm::create(
                                      "RoutingAlgorithm", &tr, &tr, 1, 12,
                                      1, 1,
                                      {2, 3, 4}, {4, 3, 1}, 6, settings);
                              delete ra;
                            }
                          }
                        } else if (decScheme == "threshold_weighted") {
                          for (auto& threshold : thresholds) {
                            for (auto& hopCountMode : hopCountModes) {
                              Json::Value settings;
                              settings["algorithm"] = "ugal";
                              settings["latency"] = latency;
                              settings["output_type"] = outputType;
                              settings["output_algorithm"] = outputAlg;
                              settings["hop_count_mode"] = hopCountMode;
                              settings["max_outputs"] = maxOutput;
                              settings["minimal"] = minType;
                              settings["non_minimal"] = nonMinType;
                              settings["min_all_vc_sets"] =
                                  static_cast<bool>(minAllVcSet);
                              settings["short_cut"] =
                                  static_cast<bool>(shortCut);
                              settings["independent_bias"] = ibias;
                              settings["decision_scheme"] = decScheme;
                              settings["threshold"] = threshold;
                              RoutingAlgorithmTestRouter tr(
                                  "Router", 16, 24);
                              HyperX::RoutingAlgorithm* ra =
                                  HyperX::RoutingAlgorithm::create(
                                      "RoutingAlgorithm", &tr, &tr, 1, 12,
                                      1, 1,
                                      {2, 3, 4}, {4, 3, 1}, 6, settings);
                              delete ra;
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
    }
  }
}
