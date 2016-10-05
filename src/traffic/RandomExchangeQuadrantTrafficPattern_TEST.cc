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
#include "traffic/RandomExchangeQuadrantTrafficPattern.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <unordered_set>
#include <vector>

#include "network/cube/util.h"
#include "test/TestSetup_TEST.h"

TEST(RandomExchangeQuadrantTrafficPattern, evenSpread) {
  TestSetup test(1, 1, 0xBAADF00D);
  Json::Value settings;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);

  u32 dimensions = 2;
  std::vector<u32> widths = {4, 4};
  u32 concentration = 4;

  const u32 numTerminals = 4 * 4 * 4;
  const u32 kRounds = 10000;
  const bool DEBUG = false;

  std::vector<RandomExchangeQuadrantTrafficPattern*> tps(numTerminals);
  for (u32 idx = 0; idx < numTerminals; idx++) {
    tps.at(idx) = new RandomExchangeQuadrantTrafficPattern(
        "TP_"+std::to_string(idx), nullptr, numTerminals, idx, settings);
  }

  std::vector<std::vector<u32>> vals(
      numTerminals, std::vector<u32>(numTerminals, 0u));

  for (u32 cnt = 0; cnt < numTerminals * kRounds; cnt++) {
    for (u32 idx = 0; idx < numTerminals; idx++) {
      u32 num = tps.at(idx)->nextDestination();
      vals.at(idx).at(num)++;
    }
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    if (DEBUG) {
      printf("%s\n", strop::vecString<u32>(vals.at(idx)).c_str());
    }

    u32 idxQuadrant = 0;
    std::vector<u32> idxAddr;
    Cube::computeTerminalAddress(idx, widths, concentration, &idxAddr);
    for (u32 i = 0; i < dimensions; ++i) {
      if (idxAddr.at(i + 1) >= widths.at(i) / 2) {
        idxQuadrant += (1 << i);
      }
    }
    std::unordered_set<u32> val;

    f64 sum = 0;
    for (u32 bkt = 0; bkt < numTerminals; bkt++) {
      u32 bktQuadrant = 0;
      std::vector<u32> bktAddr;
      Cube::computeTerminalAddress(bkt, widths, concentration, &bktAddr);
      for (u32 i = 0; i < dimensions; ++i) {
        if (bktAddr.at(i + 1) >= (widths.at(i) / 2)) {
          bktQuadrant += (1 << i);
        }
      }

      if ((bktQuadrant + idxQuadrant) == 3) {
        // copy out count in valid region
        val.insert(vals.at(idx).at(bkt));
        sum += vals.at(idx).at(bkt);
      } else {
        // verify invalid region
        assert(vals.at(idx).at(bkt) == 0);
      }
    }

    f64 mean = sum / (numTerminals / 4);

    sum = 0;
    for (const auto& p : val) {
      f64 c = (static_cast<f64>(p) - mean);
      c *= c;
      sum += c;
    }

    f64 stdDev = std::sqrt(sum / (numTerminals / 4));
    // printf("stdDev = %f\n", stdDev);
    stdDev /= kRounds;
    // printf("relStdDev = %f\n", stdDev);

    if (DEBUG) {
      printf("%u: %f %f\n", idx, mean, stdDev);
    }

    ASSERT_EQ(mean, kRounds * 4);
    ASSERT_LE(stdDev, kRounds * 4 * 0.02);
  }

  for (u32 idx = 0; idx < numTerminals; idx++) {
    delete tps.at(idx);
  }
}
