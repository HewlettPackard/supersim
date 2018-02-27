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
#include "network/fattree/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include <vector>

TEST(FatTree, translateInterfaceIdToAddress) {
  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  const u32 numLevels = 4;
  std::vector<u32> terminalsPerGroup = {4, 12, 24, 48};

  id = 0;
  exp = {0, 0, 0, 0};
  FatTree::translateInterfaceIdToAddress(numLevels, terminalsPerGroup,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {1, 0, 0, 0};
  FatTree::translateInterfaceIdToAddress(numLevels, terminalsPerGroup,
                                            id, &act);
  ASSERT_EQ(act, exp);

  id = 20;
  exp = {0, 2, 1, 0};
  FatTree::translateInterfaceIdToAddress(numLevels, terminalsPerGroup,
                                            id, &act);
  ASSERT_EQ(act, exp);

  id = 47;
  exp = {3, 2, 1, 1};
  FatTree::translateInterfaceIdToAddress(numLevels, terminalsPerGroup,
                                            id, &act);
  ASSERT_EQ(act, exp);
}

TEST(FatTree, translateInterfaceAddressToId) {
  std::vector<u32> addr;

  const u32 numLevels = 4;
  const std::vector<u32> terminalsPerGroup = {4, 12, 24, 48};

  addr = {0, 0, 0, 0};
  ASSERT_EQ(0u, FatTree::translateInterfaceAddressToId(
      numLevels, terminalsPerGroup, &addr));

  addr = {1, 0, 0, 0};
  ASSERT_EQ(1u, FatTree::translateInterfaceAddressToId(
      numLevels, terminalsPerGroup, &addr));

  addr = {0, 2, 1, 0};;
  ASSERT_EQ(20u, FatTree::translateInterfaceAddressToId(
      numLevels, terminalsPerGroup, &addr));

  addr = {3, 2, 0, 1};;
  ASSERT_EQ(35u, FatTree::translateInterfaceAddressToId(
      numLevels, terminalsPerGroup, &addr));

  addr = {3, 2, 1, 1};
  ASSERT_EQ(47u, FatTree::translateInterfaceAddressToId(
      numLevels, terminalsPerGroup, &addr));
}

TEST(FatTree, translateRouterIdToAddress) {
  std::vector<u32> act;
  std::vector<u32> exp;
  u32 id;

  const u32 numLevels = 4;
  const  std::vector<u32> rowRouters = {128, 64, 32, 10};

  id = 0;
  exp = {0, 0};
  FatTree::translateRouterIdToAddress(numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 1;
  exp = {0, 1};
  FatTree::translateRouterIdToAddress(numLevels, rowRouters,
                                         id, &act);
  ASSERT_EQ(act, exp);

  id = 130;
  exp = {1, 2};
  FatTree::translateRouterIdToAddress(numLevels, rowRouters,
                                      id, &act);
  ASSERT_EQ(act, exp);

  id = 223;
  exp = {2, 31};
  FatTree::translateRouterIdToAddress(numLevels, rowRouters,
                                      id, &act);
  ASSERT_EQ(act, exp);

  id = 230;
  exp = {3, 6};
  FatTree::translateRouterIdToAddress(numLevels, rowRouters,
                                      id, &act);
  ASSERT_EQ(act, exp);
}

TEST(FatTree, translateRouterAddressToId) {
  std::vector<u32> addr;

  const u32 numLevels = 4;
  const  std::vector<u32> rowRouters = {128, 64, 32, 10};

  addr = {0, 0};
  ASSERT_EQ(0u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));

  addr = {0, 1};
  ASSERT_EQ(1u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));

  addr = {1, 42};
  ASSERT_EQ(170u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));

  addr = {2, 0};
  ASSERT_EQ(192u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));

  addr = {2, 31};
  ASSERT_EQ(223u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));

  addr = {3, 1};
  ASSERT_EQ(225u, FatTree::translateRouterAddressToId(
      numLevels, rowRouters, &addr));
}

TEST(FatTree, computeMinimalHops) {
  std::vector<u32> src;
  std::vector<u32> dst;
  u32 exp;

  src = {0, 0};
  dst = {0, 1};
  exp = 3;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));

  src = {0, 0};
  dst = {1, 0};
  exp = 1;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));

  src = {0, 0, 0};
  dst = {2, 0, 0};
  exp = 1;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));

  src = {0, 0, 0};
  dst = {2, 2, 0};
  exp = 3;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));

  src = {0, 0, 0};
  dst = {2, 2, 2};
  exp = 5;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));


  src = {0, 0, 0, 0};
  dst = {2, 2, 2, 2};
  exp = 7;
  ASSERT_EQ(exp, FatTree::computeMinimalHops(&src, &dst));
}
