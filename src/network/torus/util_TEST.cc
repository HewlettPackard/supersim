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
#include "network/torus/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(Torus, computeMinimalHops) {
  std::vector<u32> src;
  std::vector<u32> dst;
  std::vector<u32> widths;
  u32 exp;
  u32 dimensions;

  src = {2, 0};
  dst = {0, 1};
  widths = {4};
  dimensions = widths.size();
  exp = 2;
  ASSERT_EQ(exp, Torus::computeMinimalHops(&src, &dst, dimensions, widths));

  src = {0, 0, 0};
  dst = {0, 2, 2};
  widths = {3, 3};
  dimensions = widths.size();
  exp = 3;
  ASSERT_EQ(exp, Torus::computeMinimalHops(&src, &dst, dimensions, widths));

  src = {0, 1, 0, 0};
  dst = {0, 2, 2, 2};
  widths = {3, 3, 3};
  dimensions = widths.size();
  exp = 4;
  ASSERT_EQ(exp, Torus::computeMinimalHops(&src, &dst, dimensions, widths));
}
