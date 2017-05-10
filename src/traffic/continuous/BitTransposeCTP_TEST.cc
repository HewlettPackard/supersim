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
#include "traffic/continuous/BitTransposeCTP.h"

#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include "test/TestSetup_TEST.h"

TEST(BitTransposeCTP, simple) {
  TestSetup test(1, 1, 0xBAADF00D);

  u32 src, dst;
  Json::Value settings;
  BitTransposeCTP* tp;
  u32 numTerminals = 16;
  std::map<u32, u32> pairs = {
    {0, 0},
    {1, 4},
    {2, 8},
    {3, 12},
    {4, 1},
    {5, 5},
    {6, 9},
    {7, 13},
    {8, 2},
    {9, 6},
    {10, 10},
    {11, 14},
    {12, 3},
    {13, 7},
    {14, 11},
    {15, 15}
  };

  for (const auto& p : pairs) {
    src = p.first;
    dst = p.second;
    tp = new BitTransposeCTP(
        "TP", nullptr, numTerminals, src, settings);
    for (u32 idx = 0; idx < 100; ++idx) {
      u32 next = tp->nextDestination();
      ASSERT_LT(next, numTerminals);
      ASSERT_EQ(next, dst);
    }
    delete tp;
  }
}
