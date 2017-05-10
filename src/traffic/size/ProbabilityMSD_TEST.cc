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
#include "traffic/size/ProbabilityMSD.h"

#include <gtest/gtest.h>
#include <json/json.h>

#include "test/TestSetup_TEST.h"
#include "traffic/size/MessageSizeDistribution.h"

TEST(ProbabilityMSD, simple) {
  TestSetup ts(123, 123, 123);

  Json::Value settings;
  settings["type"] = "probability";
  settings["message_sizes"] = Json::Value(Json::arrayValue);
  settings["size_probabilities"] = Json::Value(Json::arrayValue);

  std::unordered_map<u32, f64> probs = {{8, 0.5}, {1, 0.3}, {5, 0.2}};
  {
    u32 idx = 0;
    for (auto& p : probs) {
      settings["message_sizes"][idx] = p.first;
      settings["size_probabilities"][idx] = p.second;
      idx++;
    }
  }

  MessageSizeDistribution* msd = MessageSizeDistribution::create(
      "msd", nullptr, settings);

  std::unordered_map<u32, u32> counts;
  const u32 ROUNDS = 10000000;
  for (u32 round = 0; round < ROUNDS; round++) {
    u32 size = msd->nextMessageSize();
    counts[size]++;
  }

  for (auto& actp : counts) {
    u32 size = actp.first;
    u32 actCount = actp.second;
    f64 actPerc = (f64)actCount / ROUNDS;
    ASSERT_EQ(probs.count(size), 1u);
    f64 expPerc = probs.at(size);
    ASSERT_NEAR(actPerc, expPerc, 0.0002);
  }

  delete msd;
}

TEST(ProbabilityMSD, simple_over1) {
  TestSetup ts(123, 123, 123);

  Json::Value settings;
  settings["type"] = "probability";
  settings["message_sizes"] = Json::Value(Json::arrayValue);
  settings["size_probabilities"] = Json::Value(Json::arrayValue);

  std::unordered_map<u32, f64> probs = {{8, 5}, {1, 3}, {5, 2}};
  {
    u32 idx = 0;
    for (auto& p : probs) {
      settings["message_sizes"][idx] = p.first;
      settings["size_probabilities"][idx] = p.second;
      idx++;
    }
  }

  MessageSizeDistribution* msd = MessageSizeDistribution::create(
      "msd", nullptr, settings);

  std::unordered_map<u32, u32> counts;
  const u32 ROUNDS = 10000000;
  for (u32 round = 0; round < ROUNDS; round++) {
    u32 size = msd->nextMessageSize();
    counts[size]++;
  }

  for (auto& actp : counts) {
    u32 size = actp.first;
    u32 actCount = actp.second;
    f64 actPerc = (f64)actCount / ROUNDS;
    ASSERT_EQ(probs.count(size), 1u);
    f64 expPerc = probs.at(size) / 10;
    ASSERT_NEAR(actPerc, expPerc, 0.0002);
  }

  delete msd;
}

TEST(ProbabilityMSD, dependent) {
  TestSetup ts(123, 123, 123);

  Json::Value settings;
  settings["type"] = "probability";
  settings["message_sizes"] = Json::Value(Json::arrayValue);
  settings["size_probabilities"] = Json::Value(Json::arrayValue);
  settings["dependent_message_sizes"] = Json::Value(Json::arrayValue);
  settings["dependent_size_probabilities"] = Json::Value(Json::arrayValue);

  std::unordered_map<u32, f64> probs = {{1000, 1.0}};
  {
    u32 idx = 0;
    for (auto& p : probs) {
      settings["message_sizes"][idx] = p.first;
      settings["size_probabilities"][idx] = p.second;
      idx++;
    }
  }

  std::unordered_map<u32, f64> depProbs = {{8, 0.5}, {1, 0.3}, {5, 0.2}};
  {
    u32 idx = 0;
    for (auto& p : depProbs) {
      settings["dependent_message_sizes"][idx] = p.first;
      settings["dependent_size_probabilities"][idx] = p.second;
      idx++;
    }
  }

  MessageSizeDistribution* msd = MessageSizeDistribution::create(
      "msd", nullptr, settings);

  std::unordered_map<u32, u32> counts;
  const u32 ROUNDS = 10000000;
  for (u32 round = 0; round < ROUNDS; round++) {
    u32 size = msd->nextMessageSize(nullptr);
    counts[size]++;
  }

  for (auto& actp : counts) {
    u32 size = actp.first;
    u32 actCount = actp.second;
    f64 actPerc = (f64)actCount / ROUNDS;
    ASSERT_EQ(depProbs.count(size), 1u);
    f64 expPerc = depProbs.at(size);
    ASSERT_NEAR(actPerc, expPerc, 0.0002);
  }

  delete msd;
}
