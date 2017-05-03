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
#include "traffic/size/ProbabilityMSD.h"

#include <mut/mut.h>
#include <factory/Factory.h>

#include <algorithm>

ProbabilityMSD::ProbabilityMSD(
      const std::string& _name, const Component* _parent,
      Json::Value _settings)
    : MessageSizeDistribution(_name, _parent, _settings),
     doDependent_(_settings.isMember("dependent_message_sizes") &&
                  _settings.isMember("dependent_size_probabilities")) {
  // verify input settings
  assert(_settings.isMember("message_sizes") &&
         _settings["message_sizes"].isArray());
  assert(_settings.isMember("size_probabilities") &&
         _settings["size_probabilities"].isArray());
  assert(_settings["message_sizes"].size() > 0);
  assert(_settings["message_sizes"].size() ==
         _settings["size_probabilities"].size());
  if (!doDependent_) {
    assert(!_settings.isMember("dependent_message_sizes"));
    assert(!_settings.isMember("dependent_size_probabilities"));
  } else {
    assert(_settings.isMember("dependent_message_sizes") &&
           _settings["dependent_message_sizes"].isArray());
    assert(_settings.isMember("dependent_size_probabilities") &&
           _settings["dependent_size_probabilities"].isArray());
    assert(_settings["dependent_message_sizes"].size() > 0);
    assert(_settings["dependent_message_sizes"].size() ==
           _settings["dependent_size_probabilities"].size());
  }

  // create a probability array and message size array
  u32 size = _settings["message_sizes"].size();
  messageSizes_.resize(size);
  std::vector<f64> probabilityDistribution(size);

  // parse the message size array and the size probabilities array
  for (u32 idx = 0; idx < size; idx++) {
    u32 messageSize = _settings["message_sizes"][idx].asUInt();
    assert(messageSize > 0);
    messageSizes_.at(idx) = messageSize;
    probabilityDistribution.at(idx) =
        _settings["size_probabilities"][idx].asDouble();
  }

  // create a cumulative distribution from the probability distribution
  mut::generateCumulativeDistribution(probabilityDistribution,
                                      &cumulativeDistribution_);

  if (doDependent_) {
    // create a probability array and message size array
    u32 depSize = _settings["dependent_message_sizes"].size();
    depMessageSizes_.resize(depSize);
    std::vector<f64> depProbabilityDistribution(depSize);

    // parse the message size array and the size probabilities array
    for (u32 idx = 0; idx < depSize; idx++) {
      u32 depMessageSize = _settings["dependent_message_sizes"][idx].asUInt();
      assert(depMessageSize > 0);
      depMessageSizes_.at(idx) = depMessageSize;
      depProbabilityDistribution.at(idx) =
          _settings["dependent_size_probabilities"][idx].asDouble();
    }

    // create a cumulative distribution from the probability distribution
    mut::generateCumulativeDistribution(depProbabilityDistribution,
                                        &depCumulativeDistribution_);
  }
}

ProbabilityMSD::~ProbabilityMSD() {}

u32 ProbabilityMSD::minMessageSize() const {
  return *std::min_element(messageSizes_.cbegin(), messageSizes_.cend());
}

u32 ProbabilityMSD::maxMessageSize() const {
  return *std::max_element(messageSizes_.cbegin(), messageSizes_.cend());
}

u32 ProbabilityMSD::nextMessageSize() {
  f64 rnd = gSim->rnd.nextF64();
  u32 idx = mut::searchCumulativeDistribution(cumulativeDistribution_, rnd);
  return messageSizes_.at(idx);
}

u32 ProbabilityMSD::nextMessageSize(const Message* _msg) {
  if (doDependent_) {
    f64 rnd = gSim->rnd.nextF64();
    u32 idx = mut::searchCumulativeDistribution(
        depCumulativeDistribution_, rnd);
    return depMessageSizes_.at(idx);
  } else {
    return nextMessageSize();
  }
}

registerWithFactory("probability", MessageSizeDistribution,
                    ProbabilityMSD,
                    MESSAGESIZEDISTRIBUTION_ARGS);
