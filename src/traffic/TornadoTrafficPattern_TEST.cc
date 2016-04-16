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
#include "traffic/TornadoTrafficPattern.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include <unordered_map>
#include <utility>

#include "test/TestSetup_TEST.h"

TEST(TornadoTrafficPattern, simple) {
  TestSetup test(123, 123);

  Json::Value settings;
  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);

  const u32 TERMS = 4 * 3 * 5;
  std::map<u32, u32> tornados{
    {0, 28},
    {4, 32},
    {8, 36},
    {12, 20},
    {16, 24},
    {20, 48},
    {24, 52},
    {28, 56},
    {32, 40},
    {36, 44},
    {40, 8},
    {44, 12},
    {48, 16},
    {52, 0},
    {56, 4}
  };

  for (const auto& pair : tornados) {
    u32 self = pair.first;
    u32 torn = pair.second;

    for (u32 cc = 0; cc < 4; cc++) {
      TornadoTrafficPattern tp("TP", nullptr, TERMS, self + cc, settings);

      for (u32 idx = 0; idx < 100; idx++) {
        u32 next = tp.nextDestination();
        ASSERT_LE(next, TERMS);
        ASSERT_EQ(next, torn + cc);
      }
    }
  }
}
