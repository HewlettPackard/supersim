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
#include "traffic/continuous/NeighborCTP.h"

#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <cassert>

#include "test/TestSetup_TEST.h"

TEST(NeighborCTP, no_dimMask) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "right";

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 4},
    {4, 8},
    {8, 12},
    {12, 0}
  };

  for (u32 off = 0; off < 3; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 4 * off + conc;
        dst = p.second + 4 * 4 * off + conc;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, dimension_0_right) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "right";
  settings["enabled_dimensions"][0] = true;

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 4},
    {4, 8},
    {8, 12},
    {12, 0}
  };

  for (u32 off = 0; off < 3; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 4 * off + conc;
        dst = p.second + 4 * 4 * off + conc;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, dimension_0_left) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "left";
  settings["enabled_dimensions"][0] = true;

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 12},
    {4, 0},
    {8, 4},
    {12, 8}
  };

  for (u32 off = 0; off < 3; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 4 * off + conc;
        dst = p.second + 4 * 4 * off + conc;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, dimension_1_left) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "left";
  settings["enabled_dimensions"][1] = true;

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 32},
    {16, 0},
    {32, 16}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc;
        dst = p.second + 4 * off + conc;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, dimension_1_left_3d) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "left";
  settings["enabled_dimensions"][1] = true;

  numTerminals = 3 * 3 * 4 * 4;
  pairs = {
    {0, 32},
    {16, 0},
    {32, 16}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc;
        dst = p.second + 4 * off + conc;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, dimension_1_left_3d_1) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["direction"] = "left";
  settings["enabled_dimensions"][1] = true;

  numTerminals = 3 * 3 * 4 * 4;
  pairs = {
    {0, 32},
    {16, 0},
    {32, 16}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc + 3 * 4 * 4;
        dst = p.second + 4 * off + conc + 3 * 4 * 4;
        tp = new NeighborCTP(
            "TP", nullptr, numTerminals, src, settings);
        for (u32 idx = 0; idx < 100; ++idx) {
          u32 next = tp->nextDestination();
          ASSERT_LT(next, numTerminals);
          ASSERT_EQ(next, dst);
        }
        delete tp;
      }
    }
  }
}

TEST(NeighborCTP, 2d_left) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["direction"] = "left";

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 44},
    {4, 32},
    {8, 36},
    {12, 40},
    {16, 12},
    {20, 0},
    {24, 4},
    {28, 8},
    {32, 28},
    {36, 16},
    {40, 20},
    {44, 24}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first + conc;
      dst = p.second + conc;
      tp = new NeighborCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

TEST(NeighborCTP, 2d_right) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["direction"] = "right";

  numTerminals = 4 * 3 * 4;
  pairs = {
    {0, 20},
    {4, 24},
    {8, 28},
    {12, 16},
    {16, 36},
    {20, 40},
    {24, 44},
    {28, 32},
    {32, 4},
    {36, 8},
    {40, 12},
    {44, 0}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first + conc;
      dst = p.second + conc;
      tp = new NeighborCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}

TEST(NeighborCTP, 3d_right) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  NeighborCTP* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(4);
  settings["dimensions"][1] = Json::Value(3);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["enabled_dimensions"][2] = true;
  settings["direction"] = "right";

  numTerminals = 3 * 3 * 4 * 4;
  pairs = {
    {0, 20},
    {4, 24},
    {8, 28},
    {12, 16},
    {16, 36},
    {20, 40},
    {24, 44},
    {28, 32},
    {32, 4},
    {36, 8},
    {40, 12},
    {44, 0}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first + conc + 4 * 3 * 4;
      dst = p.second + conc + 2 * 4 * 3 * 4;
      tp = new NeighborCTP(
          "TP", nullptr, numTerminals, src, settings);
      for (u32 idx = 0; idx < 100; ++idx) {
        u32 next = tp->nextDestination();
        ASSERT_LT(next, numTerminals);
        ASSERT_EQ(next, dst);
      }
      delete tp;
    }
  }
}
