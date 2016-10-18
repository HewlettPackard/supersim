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
#include "traffic/TornadoTrafficPattern.h"

#include <bits/bits.h>
#include <gtest/gtest.h>
#include <json/json.h>
#include <prim/prim.h>

#include <cassert>

#include "test/TestSetup_TEST.h"

TEST(TornadoTrafficPattern, no_dimMask) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);

  numTerminals = 4 * 4 * 5;
  pairs = {
    {0, 8},
    {4, 12},
    {8, 16},
    {12, 0},
    {16, 4}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 5 * off + conc;
        dst = p.second + 4 * 5 * off + conc;
        tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, dimension_0) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;

  numTerminals = 4 * 4 * 5;
  pairs = {
    {0, 8},
    {4, 12},
    {8, 16},
    {12, 0},
    {16, 4}
  };

  for (u32 off = 0; off < 4; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * 5 * off + conc;
        dst = p.second + 4 * 5 * off + conc;
        tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, dimension_1) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][1] = true;

  numTerminals = 4 * 4 * 5;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc;
        dst = p.second + 4 * off + conc;
        tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, dimension_1_3d) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][1] = true;

  numTerminals = 3 * 4 * 4 * 5;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc;
        dst = p.second + 4 * off + conc;
        tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, dimension_1_3d_1) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][1] = true;

  numTerminals = 3 * 4 * 4 * 5;
  pairs = {
    {0, 20},
    {20, 40},
    {40, 60},
    {60, 0}
  };

  for (u32 off = 0; off < 5; ++off) {
    for (u32 conc = 0; conc < 4; ++conc) {
      for (const auto& p : pairs) {
        src = p.first + 4 * off + conc + 4 * 4 * 5;
        dst = p.second + 4 * off + conc + 4 * 4 * 5;
        tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, 2d) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;

  numTerminals = 4 * 4 * 5;
  pairs = {
    {0, 28},
    {4, 32},
    {8, 36},
    {12, 20},
    {16, 24},
    {20, 48},
    {24, 52},
    {28, 56},
    {32, 40},
    {36, 44},
    {40, 68},
    {44, 72},
    {48, 76},
    {52, 60},
    {56, 64},
    {60, 8},
    {64, 12},
    {68, 16},
    {72, 0},
    {76, 4}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first + conc;
      dst = p.second + conc;
      tp = new TornadoTrafficPattern(
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

TEST(TornadoTrafficPattern, 3d) {
  TestSetup test(1, 1, 0xBAADF00D);
  u32 src, dst, numTerminals;
  Json::Value settings;
  TornadoTrafficPattern* tp;
  std::map<u32, u32> pairs;

  settings["dimensions"][0] = Json::Value(5);
  settings["dimensions"][1] = Json::Value(4);
  settings["dimensions"][2] = Json::Value(3);
  settings["concentration"] = Json::Value(4);
  settings["enabled_dimensions"][0] = true;
  settings["enabled_dimensions"][1] = true;
  settings["enabled_dimensions"][2] = true;

  numTerminals = 3 * 4 * 4 * 5;
  pairs = {
    {0, 28},
    {4, 32},
    {8, 36},
    {12, 20},
    {16, 24},
    {20, 48},
    {24, 52},
    {28, 56},
    {32, 40},
    {36, 44},
    {40, 68},
    {44, 72},
    {48, 76},
    {52, 60},
    {56, 64},
    {60, 8},
    {64, 12},
    {68, 16},
    {72, 0},
    {76, 4}
  };

  for (u32 conc = 0; conc < 4; ++conc) {
    for (const auto& p : pairs) {
      src = p.first + conc + 4 * 4 * 5;
      dst = p.second + conc + 2 * 4 * 4 * 5;
      tp = new TornadoTrafficPattern(
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
