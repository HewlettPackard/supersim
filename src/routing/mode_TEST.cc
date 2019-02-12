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
#include "routing/mode.h"

#include <gtest/gtest.h>

#include "congestion/Congestion_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(RoutingMode, portAveMinMax) {
  TestSetup ts(1, 1, 1, 123);

  u32 numPorts = 2;
  u32 numVcs = 3;
  std::vector<f64> congestion = {0.5, 0.4, 0.1,
                                 0.2, 0.5, 0.9};

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
      std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);

  Json::Value sensorSettings;
  sensorSettings["granularity"] = 0;
  sensorSettings["minimum"] = 0;
  sensorSettings["offset"] = 0;
  CongestionTestSensor sensor(
      "Sensor", &router, &router, sensorSettings, &congestion);

  router.setCongestionSensor(&sensor);

  ASSERT_EQ(averagePortCongestion(&router, 0, 0, 0), (0.5 + 0.4 + 0.1) / 3.0);
  ASSERT_EQ(averagePortCongestion(&router, 0, 0, 1), (0.2 + 0.5 + 0.9) / 3.0);

  ASSERT_EQ(minimumPortCongestion(&router, 0, 0, 0), 0.1);
  ASSERT_EQ(minimumPortCongestion(&router, 0, 0, 1), 0.2);

  ASSERT_EQ(maximumPortCongestion(&router, 0, 0, 0), 0.5);
  ASSERT_EQ(maximumPortCongestion(&router, 0, 0, 1), 0.9);

  ASSERT_EQ(portCongestion(RoutingMode::kPortAve,
                           &router, 0, 0, 0), (0.5 + 0.4 + 0.1) / 3.0);
  ASSERT_EQ(portCongestion(RoutingMode::kPortAve,
                           &router, 0, 0, 1), (0.2 + 0.5 + 0.9) / 3.0);

  ASSERT_EQ(portCongestion(RoutingMode::kPortMin,
                           &router, 0, 0, 0), 0.1);
  ASSERT_EQ(portCongestion(RoutingMode::kPortMin,
                           &router, 0, 0, 1), 0.2);

  ASSERT_EQ(portCongestion(RoutingMode::kPortMax,
                           &router, 0, 0, 0), 0.5);
  ASSERT_EQ(portCongestion(RoutingMode::kPortMax,
                           &router, 0, 0, 1), 0.9);
}

TEST(RoutingMode, modeOrdering) {
  // this ensures the behavior in 'congestionModeIsPort'
  ASSERT_LT(RoutingMode::kVc, RoutingMode::kPortAve);
  ASSERT_LT(RoutingMode::kPortAve, RoutingMode::kPortMin);
  ASSERT_LT(RoutingMode::kPortMin, RoutingMode::kPortMax);
}

TEST(RoutingMode, modeIsPort) {
  ASSERT_FALSE(routingModeIsPort(RoutingMode::kVc));
  ASSERT_TRUE(routingModeIsPort(RoutingMode::kPortAve));
  ASSERT_TRUE(routingModeIsPort(RoutingMode::kPortMin));
  ASSERT_TRUE(routingModeIsPort(RoutingMode::kPortMax));
}
