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
#include "traffic/continuous/LoopbackCTP.h"

#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include "test/TestSetup_TEST.h"

TEST(LoopbackCTP, self) {
  TestSetup test(123, 123, 456789);

  const u32 TOTAL = 5000;
  const u32 ME = 50;
  const u32 ROUNDS = 5000000;
  Json::Value settings;
  settings["send_to_self"] = true;
  LoopbackCTP tp("TP", nullptr, TOTAL, ME, settings);

  for (u32 idx = 0; idx < ROUNDS; idx++) {
    u32 num = tp.nextDestination();
    ASSERT_EQ(num, ME);
  }
}
