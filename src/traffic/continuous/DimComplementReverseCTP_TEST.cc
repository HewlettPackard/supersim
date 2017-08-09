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
#include "traffic/continuous/DimComplementReverseCTP.h"

#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <cassert>

#include "test/TestSetup_TEST.h"

TEST(DimComplementReverseCTP, simple) {
  TestSetup test(1, 1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  DimComplementReverseCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(3);
  settings["dimensions"][1] = Json::Value(3);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);

  numTerminals = 4 * 3 * 3 * 3;

  pairs = {
    {0, 26},
    {1, 17},
    {2, 8},
    {3, 23},
    {4, 14},
    {5, 5},
    {6, 20},
    {7, 11},
    {8, 2},
    {9, 25},
    {10, 16},
    {11, 7},
    {12, 22},
    {13, 13},
    {14, 4},
    {15, 19},
    {16, 10},
    {17, 1},
    {18, 24},
    {19, 15},
    {20, 6},
    {21, 21},
    {22, 12},
    {23, 3},
    {24, 18},
    {25, 9},
    {26, 0}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first * 4 + conc;
      dst = p.second * 4 + conc;
      tp = new DimComplementReverseCTP(
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
