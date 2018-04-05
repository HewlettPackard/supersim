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
#include "traffic/continuous/LocalRemoteRandomCTP.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <tuple>
#include <vector>

#include "test/TestSetup_TEST.h"

TEST(LocalRemoteRandomCTP, full) {
  const u32 TESTS = 10000000;
  const bool DEBUG = false;

  std::vector<std::tuple<u32, u32, u32, f64> > setups;
  setups.push_back(std::make_tuple(10, 2, 6, 0.89));
  setups.push_back(std::make_tuple(50, 10, 47, 0.45));

  for (auto t : setups) {
    const u32 NSIZE = std::get<0>(t);
    const u32 BSIZE = std::get<1>(t);
    const u32 ME = std::get<2>(t);
    const f64 PROB = std::get<3>(t);
    if (DEBUG) {
      printf("NSIZE=%u BSIZE=%u ME=%u PROB=%f\n",
             NSIZE, BSIZE, ME, PROB);
    }

    assert(NSIZE % BSIZE == 0);
    const u32 BLOCK = ME / BSIZE;

    TestSetup ts(1, 1, 1, 0xDEAD * 123 + 0xBEEF);

    Json::Value settings;
    settings["block_size"] = BSIZE;
    settings["local_probability"] = PROB;

    LocalRemoteRandomCTP* tp =  new LocalRemoteRandomCTP(
        "TP", nullptr, NSIZE, ME, settings);

    std::vector<u32> counts(NSIZE, 0);
    for (u32 test = 0; test < TESTS; test++) {
      counts.at(tp->nextDestination())++;
    }

    u32 localCount = 0;
    u32 localDsts = 0;
    std::vector<u32> localCounts;
    u32 remoteCount = 0;
    u32 remoteDsts = 0;
    std::vector<u32> remoteCounts;
    for (u32 dst = 0; dst < NSIZE; dst++) {
      if (dst / BSIZE == BLOCK) {
        localCount += counts.at(dst);
        localDsts++;
        localCounts.push_back(counts.at(dst));
      } else {
        remoteCount += counts.at(dst);
        remoteDsts++;
        remoteCounts.push_back(counts.at(dst));
      }
      if (DEBUG) {
        printf("[%u] %u\n", dst, counts.at(dst));
      }
    }

    ASSERT_NEAR(localCount / (f64)TESTS, PROB, 0.001);
    ASSERT_NEAR(remoteCount / (f64)TESTS, 1.0 - PROB, 0.001);

    f64 localExp = localCount / (f64)localDsts;
    for (u32 count : localCounts) {
      f64 percent = count / localExp;
      ASSERT_NEAR(percent, 1.0, 0.01);
    }
    f64 remoteExp = remoteCount / (f64)remoteDsts;
    for (u32 count : remoteCounts) {
      f64 percent = count / remoteExp;
      ASSERT_NEAR(percent, 1.0, 0.01);
    }

    delete tp;
  }
}
