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
#include "congestion/util.h"

#include <gtest/gtest.h>

TEST(Congestion_Util, tolerance) {
  ASSERT_EQ(CONGESTION_TOLERANCE, 1e-6);
}

TEST(Congestion_Util, equal) {
  ASSERT_TRUE(congestionEqualTo(0.1, 0.1));
  ASSERT_FALSE(congestionEqualTo(0.1, 0.2));

  ASSERT_TRUE(congestionEqualTo(0.1, 0.1 + CONGESTION_TOLERANCE / 10));
  ASSERT_FALSE(congestionEqualTo(0.1, 0.1 + CONGESTION_TOLERANCE));

  ASSERT_TRUE(congestionEqualTo(0.333333, 1.0 / 3.0));
}

TEST(Congestion_Util, lessThan) {
  ASSERT_TRUE(congestionLessThan(0.1, 0.2));
  ASSERT_TRUE(congestionLessThan(0.099999, 0.1));
  ASSERT_FALSE(congestionLessThan(0.0999999, 0.1));
}

TEST(Congestion_Util, greaterThan) {
  ASSERT_TRUE(congestionGreaterThan(0.2, 0.1));
  ASSERT_TRUE(congestionGreaterThan(0.100001, 0.1));
  ASSERT_FALSE(congestionGreaterThan(0.1000001, 0.1));
}
