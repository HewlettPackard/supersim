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
#include "traffic/ScanTrafficPattern.h"

#include <json/json.h>
#include <gtest/gtest.h>
#include <mut/mut.h>
#include <prim/prim.h>
#include <strop/strop.h>

#include <vector>

#include "test/TestSetup_TEST.h"

TEST(ScanTrafficPattern, ascend_self_random) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = true;
  settings["direction"] = "ascend";
  settings["initial"] = "random";
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = U32_MAX;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    if (last != U32_MAX) {
      ASSERT_EQ(num, (last + 1) % TOTAL);
    }
    last = num;
  }
}

TEST(ScanTrafficPattern, ascend_self_321) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = true;
  settings["direction"] = "ascend";
  settings["initial"] = 321;
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = 321 - 1;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    ASSERT_EQ(num, (last + 1) % TOTAL);
    last = num;
  }
}

TEST(ScanTrafficPattern, descend_self_random) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = true;
  settings["direction"] = "descend";
  settings["initial"] = "random";
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = U32_MAX;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    if (last != U32_MAX) {
      u32 exp = last == 0 ? TOTAL - 1 : last - 1;
      ASSERT_EQ(num, exp);
    }
    last = num;
  }
}

TEST(ScanTrafficPattern, descend_self_321) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = true;
  settings["direction"] = "descend";
  settings["initial"] = 321;
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = 321 + 1;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    u32 exp = last == 0 ? TOTAL - 1 : last - 1;
    ASSERT_EQ(num, exp);
    last = num;
  }
}

TEST(ScanTrafficPattern, ascend_noself_random) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = false;
  settings["direction"] = "ascend";
  settings["initial"] = "random";
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = U32_MAX;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    if (last != U32_MAX) {
      u32 exp = last;
      do {
        exp = (exp + 1) % TOTAL;
      } while (exp == ME);
      ASSERT_EQ(num, exp);
    }
    last = num;
  }
}

TEST(ScanTrafficPattern, ascend_noself_321) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = false;
  settings["direction"] = "ascend";
  settings["initial"] = 321;
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);


  u32 last = 321 - 1;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    u32 exp = last;
    do {
      exp = (exp + 1) % TOTAL;
    } while (exp == ME);
    ASSERT_EQ(num, exp);
    last = num;
  }
}

TEST(ScanTrafficPattern, descend_noself_random) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = false;
  settings["direction"] = "descend";
  settings["initial"] = "random";
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = U32_MAX;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    if (last != U32_MAX) {
      u32 exp;
      do {
        exp = last == 0 ? TOTAL - 1 : last - 1;
        last = exp;
      } while (exp == ME);
      ASSERT_EQ(num, exp);
    }
    last = num;
  }
}

TEST(ScanTrafficPattern, descend_noself_321) {
  TestSetup test(123, 123);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = false;
  settings["direction"] = "descend";
  settings["initial"] = 321;
  ScanTrafficPattern tp("TP", nullptr, TOTAL, ME, settings);

  u32 last = 321 + 1;
  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    u32 exp;
    do {
      exp = last == 0 ? TOTAL - 1 : last - 1;
      last = exp;
    } while (exp == ME);
    ASSERT_EQ(num, exp);
  }
}
