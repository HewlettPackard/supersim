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
#include "traffic/continuous/RandomExchangeNeighborCTP.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <unordered_set>
#include <vector>

#include "network/cube/util.h"
#include "test/TestSetup_TEST.h"

TEST(RandomExchangeNeighborCTP, evenSpread_1d) {
  TestSetup test(1, 1, 0xBAADF00D);
  Json::Value settings;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;

  std::vector<u32> widths = {4, 4};

  const u32 numTerminals = 4 * 4 * 4;
  const u32 kRounds = 10000;
  const bool DEBUG = false;

  std::vector<RandomExchangeNeighborCTP*> tps(numTerminals);
  for (u32 idx = 0; idx < numTerminals; idx++) {
    tps.at(idx) = new RandomExchangeNeighborCTP(
        "TP_"+std::to_string(idx), nullptr, numTerminals, idx, settings);
  }

  std::vector<std::vector<u32>> vals(
      numTerminals, std::vector<u32>(numTerminals, 0u));

  for (u32 cnt = 0; cnt < kRounds * 2; cnt++) {
    for (u32 idx = 0; idx < numTerminals; idx++) {
      u32 num = tps.at(idx)->nextDestination();
      vals.at(idx).at(num)++;
    }
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    if (DEBUG) {
      printf("%s\n", strop::vecString<u32>(vals.at(idx)).c_str());
    }

    std::unordered_set<u32> val;

    f64 sum = 0;
    u32 nonZeroBkts = 0;
    for (u32 bkt = 0; bkt < numTerminals; bkt++) {
      if (vals.at(idx).at(bkt) > 0) {
        // copy out count in valid region
        val.insert(vals.at(idx).at(bkt));
        sum += vals.at(idx).at(bkt);
        nonZeroBkts++;
      }
    }

    ASSERT_EQ(nonZeroBkts, 2u);
    f64 mean = sum / nonZeroBkts;

    sum = 0;
    for (const auto& p : val) {
      f64 c = (static_cast<f64>(p) - mean);
      c *= c;
      sum += c;
    }

    f64 stdDev = std::sqrt(sum / nonZeroBkts);
    // printf("stdDev = %f\n", stdDev);
    stdDev /= kRounds;
    // printf("relStdDev = %f\n", stdDev);

    if (DEBUG) {
      printf("%u: %f %f\n", idx, mean, stdDev);
    }

    ASSERT_EQ(mean, kRounds);
    ASSERT_LE(stdDev, kRounds * 0.02);
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    delete tps.at(idx);
  }
}

TEST(RandomExchangeNeighborCTP, evenSpread_1d_all_terminals) {
  TestSetup test(1, 1, 0xBAADF00D);
  Json::Value settings;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["all_terminals"] = true;

  std::vector<u32> widths = {4, 4};

  const u32 numTerminals = 4 * 4 * 4;
  const u32 kRounds = 10000;
  const bool DEBUG = false;

  std::vector<RandomExchangeNeighborCTP*> tps(numTerminals);
  for (u32 idx = 0; idx < numTerminals; idx++) {
    tps.at(idx) = new RandomExchangeNeighborCTP(
        "TP_"+std::to_string(idx), nullptr, numTerminals, idx, settings);
  }

  std::vector<std::vector<u32>> vals(
      numTerminals, std::vector<u32>(numTerminals, 0u));

  for (u32 cnt = 0; cnt < kRounds * 8; cnt++) {
    for (u32 idx = 0; idx < numTerminals; idx++) {
      u32 num = tps.at(idx)->nextDestination();
      vals.at(idx).at(num)++;
    }
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    if (DEBUG) {
      printf("%s\n", strop::vecString<u32>(vals.at(idx)).c_str());
    }

    std::unordered_set<u32> val;

    f64 sum = 0;
    u32 nonZeroBkts = 0;
    for (u32 bkt = 0; bkt < numTerminals; bkt++) {
      if (vals.at(idx).at(bkt) > 0) {
        // copy out count in valid region
        val.insert(vals.at(idx).at(bkt));
        sum += vals.at(idx).at(bkt);
        nonZeroBkts++;
      }
    }

    ASSERT_EQ(nonZeroBkts, 8u);
    f64 mean = sum / nonZeroBkts;

    sum = 0;
    for (const auto& p : val) {
      f64 c = (static_cast<f64>(p) - mean);
      c *= c;
      sum += c;
    }

    f64 stdDev = std::sqrt(sum / nonZeroBkts);
    // printf("stdDev = %f\n", stdDev);
    stdDev /= kRounds;
    // printf("relStdDev = %f\n", stdDev);

    if (DEBUG) {
      printf("%u: %f %f\n", idx, mean, stdDev);
    }

    ASSERT_EQ(mean, kRounds);
    ASSERT_LE(stdDev, kRounds * 0.02);
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    delete tps.at(idx);
  }
}

TEST(RandomExchangeNeighborCTP, evenSpread_2d_all_terminals) {
  TestSetup test(1, 1, 0xBAADF00D);
  Json::Value settings;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["all_terminals"] = true;

  std::vector<u32> widths = {4, 4};

  const u32 numTerminals = 4 * 4 * 4;
  const u32 kRounds = 10000;
  const bool DEBUG = false;

  std::vector<RandomExchangeNeighborCTP*> tps(numTerminals);
  for (u32 idx = 0; idx < numTerminals; idx++) {
    tps.at(idx) = new RandomExchangeNeighborCTP(
        "TP_"+std::to_string(idx), nullptr, numTerminals, idx, settings);
  }

  std::vector<std::vector<u32>> vals(
      numTerminals, std::vector<u32>(numTerminals, 0u));

  for (u32 cnt = 0; cnt < kRounds * 16; cnt++) {
    for (u32 idx = 0; idx < numTerminals; idx++) {
      u32 num = tps.at(idx)->nextDestination();
      vals.at(idx).at(num)++;
    }
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    if (DEBUG) {
      printf("%s\n", strop::vecString<u32>(vals.at(idx)).c_str());
    }

    std::unordered_set<u32> val;

    f64 sum = 0;
    u32 nonZeroBkts = 0;
    for (u32 bkt = 0; bkt < numTerminals; bkt++) {
      if (vals.at(idx).at(bkt) > 0) {
        // copy out count in valid region
        val.insert(vals.at(idx).at(bkt));
        sum += vals.at(idx).at(bkt);
        nonZeroBkts++;
      }
    }

    ASSERT_EQ(nonZeroBkts, 16u);
    f64 mean = sum / nonZeroBkts;

    sum = 0;
    for (const auto& p : val) {
      f64 c = (static_cast<f64>(p) - mean);
      c *= c;
      sum += c;
    }

    f64 stdDev = std::sqrt(sum / nonZeroBkts);
    // printf("stdDev = %f\n", stdDev);
    stdDev /= kRounds;
    // printf("relStdDev = %f\n", stdDev);

    if (DEBUG) {
      printf("%u: %f %f\n", idx, mean, stdDev);
    }

    ASSERT_EQ(mean, kRounds);
    ASSERT_LE(stdDev, kRounds * 0.02);
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    delete tps.at(idx);
  }
}
