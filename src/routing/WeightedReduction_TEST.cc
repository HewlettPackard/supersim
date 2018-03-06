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
#include "routing/WeightedReduction.h"

#include <json/json.h>
#include <gtest/gtest.h>

#include "architecture/PortedDevice_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(WeightedReduction, vc) {
  for (u32 i = 0; i < 2; i++) {
    TestSetup ts(1, 1, 1, 12345+i);

    TestPortedDevice dev(4, 6);
    Json::Value settings;
    settings["max_outputs"] = 1;
    settings["congestion_bias"] = 0.0;
    settings["independent_bias"] = 0.0;
    settings["non_minimal_weight_func"] = "regular";
    WeightedReduction red(
        "Reduction", nullptr, &dev, RoutingMode::kVc, false, settings);

    {
      red.add(1, 4, 4, 0.666666);  // winner
      red.add(0, 3, 4, 0.7);
      red.add(2, 5, 4, 0.9);
      red.add(3, 1, 8, 0.333333);  // loser

      bool allMin;
      const auto* o = red.reduce(&allMin);
      ASSERT_TRUE(allMin);
      ASSERT_EQ(o->size(), 1u);
      std::tuple<u32, u32> winner = *(o->begin());
      ASSERT_EQ(std::get<0>(winner), 1u);
      ASSERT_EQ(std::get<1>(winner), 4u);
    }

    {
      red.add(1, 4, 4, 0.666666);  // loser
      red.add(0, 3, 4, 0.7);
      red.add(2, 5, 4, 0.9);
      red.add(3, 1, 8, 0.3);  // winner

      bool allMin;
      const auto* o = red.reduce(&allMin);
      ASSERT_FALSE(allMin);
      ASSERT_EQ(o->size(), 1u);
      std::tuple<u32, u32> winner = *(o->begin());
      ASSERT_EQ(std::get<0>(winner), 3u);
      ASSERT_EQ(std::get<1>(winner), 1u);
    }
  }
}
