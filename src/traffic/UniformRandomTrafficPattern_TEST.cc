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
#include "traffic/UniformRandomTrafficPattern.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <vector>

#include "test/TestSetup_TEST.h"

TEST(UniformRandomTrafficPattern, evenSpread) {
  /*
   * This tests single patterns as well as any side effects from all
   * patterns sharing the same random number generator. This is a
   * two-dimensional test (literally).
   */
  const u32 TPS = 40;
  const u32 DUP = 10000;
  const bool DEBUG = false;

  TestSetup test(1234, 1234);

  Json::Value settings;
  settings["send_to_self"] = true;
  std::vector<UniformRandomTrafficPattern*> tps(TPS);
  for (u32 idx = 0; idx < TPS; idx++) {
    tps.at(idx) = new UniformRandomTrafficPattern(
        "TP_"+std::to_string(idx), nullptr, TPS, idx, settings);
  }

  std::vector<std::vector<u32>> vals(TPS, std::vector<u32>(TPS, 0u));

  for (u32 cnt = 0; cnt < TPS * DUP; cnt++) {
    for (u32 idx = 0; idx < TPS; idx++) {
      u32 num = tps.at(idx)->nextDestination();
      vals.at(idx).at(num)++;
    }
  }

  for (u32 idx = 0; idx < TPS; idx++) {
    if (DEBUG && idx == 0) {
      printf("%s\n", strop::vecString<u32>(vals.at(idx)).c_str());
    }

    f64 mean = mut::arithmeticMean<u32>(vals.at(idx));
    f64 variance = mut::variance<u32>(vals.at(idx), mean);
    f64 stddev = mut::standardDeviation<u32>(variance);
    if (DEBUG) {
      printf("%u: %f %f %f\n", idx, mean, variance, stddev);
    }

    ASSERT_EQ(mean, DUP);
    ASSERT_LE(stddev, DUP * 0.02);
  }

  for (u32 idx = 0; idx < TPS; idx++) {
    delete tps.at(idx);
  }
}

TEST(UniformRandomTrafficPattern, noSelf) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = false;
  UniformRandomTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    ASSERT_NE(num, ME);
  }
}
