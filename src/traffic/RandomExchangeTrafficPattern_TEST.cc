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
#include "traffic/RandomExchangeTrafficPattern.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <vector>

#include "test/TestSetup_TEST.h"

TEST(RandomExchangeTrafficPattern, evenSpread) {
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
  std::vector<RandomExchangeTrafficPattern*> tps(TPS);
  for (u32 idx = 0; idx < TPS; idx++) {
    tps.at(idx) = new RandomExchangeTrafficPattern(
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

    bool imlower = idx < (TPS / 2);
    std::vector<u32> val(TPS / 2, 0);
    for (u32 bkt = 0; bkt < TPS; bkt++) {
      bool thislower = bkt < (TPS / 2);
      if ((imlower && !thislower) ||
          (!imlower && thislower)) {
        // copy out count in valid region
        val.at(bkt % (TPS / 2)) = vals.at(idx).at(bkt);
      } else {
        // veridy invalid region
        assert(vals.at(idx).at(bkt) == 0);
      }
    }

    f64 mean = mut::arithmeticMean<u32>(val);
    f64 variance = mut::variance<u32>(val, mean);
    f64 stddev = mut::standardDeviation<u32>(variance);
    if (DEBUG) {
      printf("%u: %f %f %f\n", idx, mean, variance, stddev);
    }

    ASSERT_EQ(mean, DUP * 2);
    ASSERT_LE(stddev, DUP * 2 * 0.02);
  }

  for (u32 idx = 0; idx < TPS; idx++) {
    delete tps.at(idx);
  }
}
