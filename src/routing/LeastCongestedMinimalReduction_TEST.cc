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
#include "routing/LeastCongestedMinimalReduction.h"

#include <json/json.h>
#include <gtest/gtest.h>

#include "architecture/PortedDevice_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(LeastCongestedMinimalReduction, vc) {
  for (u32 i = 0; i < 2; i++) {
    TestSetup ts(1, 1, 1, 12345+i);

    const u32 ROUNDS = 100000;

    TestPortedDevice dev(4, 6);
    Json::Value settings;
    settings["max_outputs"] = 1;
    LeastCongestedMinimalReduction red(
        "Reduction", nullptr, &dev, RoutingMode::kVc, false, settings);

    s32 bias = 0;
    for (u32 cnt = 0; cnt < ROUNDS; cnt++) {
      red.add(1, 4, 7, 0.3333333);  // this
      red.add(0, 3, 7, 0.5);
      red.add(2, 5, 7, 1.0/3.0);  // this
      red.add(3, 1, 8, 0.0);

      bool allMin;
      const auto* o = red.reduce(&allMin);
      ASSERT_TRUE(allMin);
      ASSERT_EQ(o->size(), 1u);
      std::tuple<u32, u32> winner = *(o->begin());

      if (std::get<0>(winner) == 1) {
        ASSERT_EQ(std::get<1>(winner), 4u);
        bias++;
      } else if (std::get<0>(winner) == 2) {
        ASSERT_EQ(std::get<1>(winner), 5u);
        bias--;
      } else {
        ASSERT_TRUE(false);
      }
    }
    ASSERT_NEAR((f64)bias, 0.0, ROUNDS * 0.01);
  }
}
