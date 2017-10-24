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
#include "traffic/continuous/RandomBlockOutCTP.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <vector>

#include "test/TestSetup_TEST.h"

TEST(RandomBlockOutCTP, all) {
  TestSetup ts(1, 1, 1, 0xDEADBEEF);

  const u32 NUMTERMINALS = 1000;
  const u32 BLOCKSIZE = 100;

  std::vector<RandomBlockOutCTP*> ctps(NUMTERMINALS, nullptr);
  for (u32 ctp = 0; ctp < NUMTERMINALS; ctp++) {
    Json::Value settings;
    settings["block_size"] = BLOCKSIZE;
    RandomBlockOutCTP* tp =  new RandomBlockOutCTP(
        "TP_" + std::to_string(ctp), nullptr, NUMTERMINALS, ctp, settings);
    ctps.at(ctp) = tp;
  }

  std::vector<u32> destCounts(NUMTERMINALS, 0);

  const u32 COUNT = 30000;
  for (u32 cnt = 0; cnt < COUNT; cnt++) {
    for (u32 ctp = 0; ctp < NUMTERMINALS; ctp++) {
      u32 blockBase = (ctp / BLOCKSIZE) * BLOCKSIZE;

      u32 dest = ctps.at(ctp)->nextDestination();
      bool ok = dest < blockBase || dest >= (blockBase + BLOCKSIZE);
      ASSERT_TRUE(ok);

      destCounts.at(dest)++;
    }
  }

  f64 mean = mut::arithmeticMean<u32>(destCounts);
  f64 variance = mut::variance<u32>(destCounts, mean);
  f64 stddev = mut::standardDeviation<u32>(variance);
  if (false) {
    printf("%f %f %f\n", mean, variance, stddev);
  }

  f64 expMean = COUNT;
  ASSERT_EQ(mean, expMean);
  ASSERT_LE(stddev, expMean * 0.02);
}
