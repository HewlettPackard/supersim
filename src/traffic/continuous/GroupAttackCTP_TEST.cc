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
#include "traffic/continuous/GroupAttackCTP.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <vector>

#include "test/TestSetup_TEST.h"

TEST(GroupAttackCTP, half_permutation) {
  for (u32 net = 0; net < 100; net++) {
    TestSetup ts(1, 1, 1, 0xDEAD * net + 0xBEEF);

    u32 groupCount = gSim->rnd.nextU64(1, 20);
    u32 groupSize = gSim->rnd.nextU64(1, 20);
    u32 concentration = gSim->rnd.nextU64(1, 20);

    for (u32 test = 0; test < 100; test++) {
      Json::Value settings;
      settings["group_size"] = groupSize;
      settings["concentration"] = concentration;
      settings["random"] = false;
      settings["mode"] = "half";

      u32 selfGroup = gSim->rnd.nextU64(0, groupCount - 1);
      u32 selfLocal = gSim->rnd.nextU64(0, groupSize - 1);
      u32 selfConc = gSim->rnd.nextU64(0, concentration - 1);

      u32 self = ((selfGroup * groupSize * concentration) +
                  (selfLocal * concentration) +
                  (selfConc));

      GroupAttackCTP* tp =  new GroupAttackCTP(
          "TP", nullptr, groupCount * groupSize * concentration, self,
          settings);

      u32 expDestGroup = (selfGroup + (groupCount / 2)) % groupCount;
      u32 expDestLocal = selfLocal;
      u32 expDestConc = selfConc;
      u32 expDest = ((expDestGroup * groupSize * concentration) +
                     (expDestLocal * concentration) +
                     (expDestConc));

      for (u32 cnt = 0; cnt < 100; cnt++) {
        u32 dest = tp->nextDestination();
        ASSERT_EQ(dest, expDest);
      }

      delete tp;
    }
  }
}

TEST(GroupAttackCTP, opposite_permutation) {
  for (u32 net = 0; net < 100; net++) {
    TestSetup ts(1, 1, 1, 0xDEAD * net + 0xBEEF);

    u32 groupCount = gSim->rnd.nextU64(1, 20);
    u32 groupSize = gSim->rnd.nextU64(1, 20);
    u32 concentration = gSim->rnd.nextU64(1, 20);

    for (u32 test = 0; test < 100; test++) {
      Json::Value settings;
      settings["group_size"] = groupSize;
      settings["concentration"] = concentration;
      settings["random"] = false;
      settings["mode"] = "opposite";

      u32 selfGroup = gSim->rnd.nextU64(0, groupCount - 1);
      u32 selfLocal = gSim->rnd.nextU64(0, groupSize - 1);
      u32 selfConc = gSim->rnd.nextU64(0, concentration - 1);

      u32 self = ((selfGroup * groupSize * concentration) +
                  (selfLocal * concentration) +
                  (selfConc));

      GroupAttackCTP* tp =  new GroupAttackCTP(
          "TP", nullptr, groupCount * groupSize * concentration, self,
          settings);

      u32 expDestGroup = (groupCount - 1) - selfGroup;
      u32 expDestLocal = selfLocal;
      u32 expDestConc = selfConc;
      u32 expDest = ((expDestGroup * groupSize * concentration) +
                     (expDestLocal * concentration) +
                     (expDestConc));

      for (u32 cnt = 0; cnt < 100; cnt++) {
        u32 dest = tp->nextDestination();
        ASSERT_EQ(dest, expDest);
      }

      delete tp;
    }
  }
}


TEST(GroupAttackCTP, offset_permutation) {
  for (u32 net = 0; net < 100; net++) {
    TestSetup ts(1, 1, 1, 0xDEAD * net + 0xBEEF);

    u32 groupCount = gSim->rnd.nextU64(1, 10);
    u32 groupSize = gSim->rnd.nextU64(1, 10);
    u32 concentration = gSim->rnd.nextU64(1, 10);
    for (s32 offset = -1; abs(offset) < groupCount; offset ++) {
      for (u32 test = 0; test < 100; test++) {
        Json::Value settings;
        settings["group_size"] = groupSize;
        settings["concentration"] = concentration;
        settings["random"] = false;
        settings["mode"] = offset;

        u32 selfGroup = gSim->rnd.nextU64(0, groupCount - 1);
        u32 selfLocal = gSim->rnd.nextU64(0, groupSize - 1);
        u32 selfConc = gSim->rnd.nextU64(0, concentration - 1);

        u32 self = ((selfGroup * groupSize * concentration) +
                    (selfLocal * concentration) +
                    (selfConc));

        GroupAttackCTP* tp =  new GroupAttackCTP(
            "TP", nullptr, groupCount * groupSize * concentration, self,
            settings);

        s32 destGroup = ((s32)selfGroup +
                         ((s32)groupCount + offset)) % (s32)groupCount;
        if (destGroup < 0) {
          destGroup += groupCount;
        }

        u32 expDestGroup = (u32)destGroup;
        u32 expDestLocal = selfLocal;
        u32 expDestConc = selfConc;
        u32 expDest = ((expDestGroup * groupSize * concentration) +
                       (expDestLocal * concentration) +
                       (expDestConc));

        for (u32 cnt = 0; cnt < 100; cnt++) {
          u32 dest = tp->nextDestination();
          ASSERT_EQ(dest, expDest);
        }

        delete tp;
      }
    }
  }
}

TEST(GroupAttackCTP, random) {
  const u32 DEBUG = 0;  // 0=off 1=min 2=max

  f64 wc = 0.0;

  const u32 CONFIGS = 100;
  const u32 TESTS = 10;
  const u32 ROUNDS = 10000;

  for (u32 net = 0; net < CONFIGS; net++) {
    TestSetup ts(1, 1, 1, 0xDEAD * net + 0xBEEF);

    u32 groupCount;
    u32 groupSize;
    u32 concentration;

    do {
      groupCount = gSim->rnd.nextU64(1, 20);
      groupSize = gSim->rnd.nextU64(1, 20);
      concentration = gSim->rnd.nextU64(1, 20);
    } while ((groupSize * concentration) < 5);

    if (DEBUG > 1) {
      printf("config: gc=%u gs=%u c=%u\n",
             groupCount, groupSize, concentration);
    }
    for (u32 test = 0; test < TESTS; test++) {
      Json::Value settings;
      settings["group_size"] = groupSize;
      settings["concentration"] = concentration;
      settings["random"] = true;
      settings["mode"] = "half";

      u32 selfGroup = gSim->rnd.nextU64(0, groupCount - 1);
      u32 selfLocal = gSim->rnd.nextU64(0, groupSize - 1);
      u32 selfConc = gSim->rnd.nextU64(0, concentration - 1);

      u32 self = ((selfGroup * groupSize * concentration) +
                  (selfLocal * concentration) +
                  (selfConc));

      if (DEBUG > 1) {
        printf("self: g=%u l=%u c=%u s=%u\n",
               selfGroup, selfLocal, selfConc, self);
      }

      GroupAttackCTP* tp = new GroupAttackCTP(
          "TP", nullptr, groupCount * groupSize * concentration, self,
          settings);

      u32 expDestGroup = (selfGroup + (groupCount / 2)) % groupCount;

      std::vector<u32> destCounts(groupSize * concentration, 0);
      for (u32 cnt = 0; cnt < ROUNDS; cnt++) {
        u32 dest = tp->nextDestination();
        u32 destGroup = dest / (groupSize * concentration);
        u32 destLocalIndex = dest % (groupSize * concentration);

        ASSERT_EQ(destGroup, expDestGroup);
        destCounts.at(destLocalIndex)++;
      }

      f64 mean = mut::arithmeticMean<u32>(destCounts);
      f64 variance = mut::variance<u32>(destCounts, mean);
      f64 stddev = mut::standardDeviation<u32>(variance);
      if (DEBUG > 1) {
        printf("%f %f %f\n", mean, variance, stddev);
        printf("%s\n", strop::vecString<u32>(destCounts).c_str());
      }

      f64 expMean = (f64)ROUNDS / (f64)(groupSize * concentration);
      ASSERT_NEAR(mean, expMean, 0.001);
      f64 relStddev = stddev / ROUNDS;
      ASSERT_LE(relStddev, 0.01);

      if (relStddev > wc) {
        wc = relStddev;
        if (DEBUG > 0) {
          printf("%f: gc=%u gs=%u c=%u\n", relStddev,
                 groupCount, groupSize, concentration);
        }
      }

      delete tp;
    }
  }
}
