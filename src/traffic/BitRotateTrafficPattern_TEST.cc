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
#include "traffic/BitRotateTrafficPattern.h"

#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include "test/TestSetup_TEST.h"

TEST(BitRotateTrafficPattern, right) {
  TestSetup test(123, 123);

  const u32 TERMS = 1 << 8;
  for (u32 idx = 0; idx < TERMS; idx++) {
    Json::Value settings;
    settings["direction"] = "right";
    BitRotateTrafficPattern tp("TP", nullptr, TERMS, idx, settings);
    for (u32 cnt = 0; cnt < 1000; cnt++) {
      u32 next = tp.nextDestination();
      ASSERT_LE(next, TERMS);
      ASSERT_EQ(next, bits::rotateRight<u32>(idx, bits::floorLog2(TERMS)));
    }
  }
}

TEST(BitRotateTrafficPattern, left) {
  TestSetup test(123, 123);

  const u32 TERMS = 1 << 8;
  for (u32 idx = 0; idx < TERMS; idx++) {
    Json::Value settings;
    settings["direction"] = "left";
    BitRotateTrafficPattern tp("TP", nullptr, TERMS, idx, settings);
    for (u32 cnt = 0; cnt < 1000; cnt++) {
      u32 next = tp.nextDestination();
      ASSERT_LE(next, TERMS);
      ASSERT_EQ(next, bits::rotateLeft<u32>(idx, bits::floorLog2(TERMS)));
    }
  }
}
