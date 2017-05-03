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
#include "traffic/size/RandomMSD.h"

#include <gtest/gtest.h>
#include <json/json.h>

#include "test/TestSetup_TEST.h"
#include "traffic/size/MessageSizeDistribution.h"

TEST(RandomMSD, simple) {
  TestSetup ts(123, 123, 123);

  const u32 MIN = 3;
  const u32 MAX = 8;

  Json::Value settings;
  settings["type"] = "random";
  settings["min_message_size"] = MIN;
  settings["max_message_size"] = MAX;

  MessageSizeDistribution* msd = MessageSizeDistribution::create(
      "msd", nullptr, settings);

  std::vector<u32> counts(MAX - MIN + 1, 0);
  const u32 ROUNDS = 10000000;
  for (u32 round = 0; round < ROUNDS; round++) {
    u32 size = msd->nextMessageSize();
    ASSERT_LE(size, MAX);
    ASSERT_GE(size, MIN);
    u32 idx = size - MIN;
    counts.at(idx)++;
  }

  for (u32 size = MIN; size <= MAX; size++) {
    u32 idx = size - MIN;
    f64 act = (f64)counts.at(idx) / ROUNDS;
    f64 exp = ((f64)ROUNDS / (MAX - MIN + 1)) / ROUNDS;
    ASSERT_NEAR(act, exp, 0.0002);
  }

  delete msd;
}

TEST(RandomMSD, dependent) {
  TestSetup ts(123, 123, 123);

  const u32 MIN = 3;
  const u32 MAX = 8;

  Json::Value settings;
  settings["type"] = "random";
  settings["min_message_size"] = 1;
  settings["max_message_size"] = 1;
  settings["dependent_min_message_size"] = MIN;
  settings["dependent_max_message_size"] = MAX;

  MessageSizeDistribution* msd = MessageSizeDistribution::create(
      "msd", nullptr, settings);

  std::vector<u32> counts(MAX - MIN + 1, 0);
  const u32 ROUNDS = 10000000;
  for (u32 round = 0; round < ROUNDS; round++) {
    u32 size = msd->nextMessageSize(nullptr);
    ASSERT_LE(size, MAX);
    ASSERT_GE(size, MIN);
    u32 idx = size - MIN;
    counts.at(idx)++;
  }

  for (u32 size = MIN; size <= MAX; size++) {
    u32 idx = size - MIN;
    f64 act = (f64)counts.at(idx) / ROUNDS;
    f64 exp = ((f64)ROUNDS / (MAX - MIN + 1)) / ROUNDS;
    ASSERT_NEAR(act, exp, 0.0002);
  }

  delete msd;
}
