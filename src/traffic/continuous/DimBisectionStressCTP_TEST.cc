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
#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <cassert>

#include "traffic/continuous/DimBisectionStressCTP.h"
#include "test/TestSetup_TEST.h"

TEST(DimBisectionStressCTP, parity) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  DimBisectionStressCTP* tp;
  std::map<u32, u32> pairs_DC, pairs_HB;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(2);
  settings["mode"] = "parity";

  numTerminals = 2 * 4 * 4;
  pairs_DC = {
    {0, 30},
    {2, 28},
    {4, 26},
    {6, 24},
    {8, 22},
    {10, 20},
    {12, 18},
    {14, 16},
    {16, 14},
    {18, 12},
    {20, 10},
    {22, 8},
    {24, 6},
    {26, 4},
    {28, 2},
    {30, 0}
  };
  pairs_HB = {
    {1, 21},
    {3, 23},
    {5, 17},
    {7, 19},
    {9, 29},
    {11, 31},
    {13, 25},
    {15, 27},
    {17, 5},
    {19, 7},
    {21, 1},
    {23, 3},
    {25, 13},
    {27, 15},
    {29, 9},
    {31, 11}
  };

  for (const auto& p : pairs_DC) {
    src = p.first;
    dst = p.second;
    tp = new DimBisectionStressCTP(
        "TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
  for (const auto& p : pairs_HB) {
    src = p.first;
    dst = p.second;
    tp = new DimBisectionStressCTP(
        "TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
}

TEST(DimBisectionStressCTP, half) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  DimBisectionStressCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(2);
  settings["mode"] = "half";

  numTerminals = 2 * 4 * 4;

  pairs = {
    {0, 15},
    {1, 14},
    {2, 13},
    {3, 12},
    {4, 11},
    {5, 10},
    {6, 9},
    {7, 8},
    {8, 2},
    {9, 3},
    {10, 0},
    {11, 1},
    {12, 6},
    {13, 7},
    {14, 4},
    {15, 5},
  };

  for (u32 conc = 0; conc < 2; ++conc) {
    for (const auto& p : pairs) {
      src = p.first * 2 + conc;
      dst = p.second * 2 + conc;
      tp = new DimBisectionStressCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

TEST(DimBisectionStressCTP, quadrant) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  DimBisectionStressCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(2);
  settings["mode"] = "quadrant";

  numTerminals = 2 * 4 * 4;

  pairs = {
    {0, 15},
    {1, 14},
    {2, 8},
    {3, 9},
    {4, 11},
    {5, 10},
    {6, 12},
    {7, 13},
    {8, 2},
    {9, 3},
    {10, 5},
    {11, 4},
    {12, 6},
    {13, 7},
    {14, 1},
    {15, 0},
  };

  for (u32 conc = 0; conc < 2; ++conc) {
    for (const auto& p : pairs) {
      src = p.first * 2 + conc;
      dst = p.second * 2 + conc;
      tp = new DimBisectionStressCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}
