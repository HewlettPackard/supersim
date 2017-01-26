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
#include "traffic/RandomMessageSizeDistribution.h"

#include <cassert>

#include "event/Simulator.h"

RandomMessageSizeDistribution::RandomMessageSizeDistribution(
    const std::string& _name, const Component* _parent,
    Json::Value _settings)
    : MessageSizeDistribution(_name, _parent),
      minMessageSize_(_settings["min_message_size"].asUInt()),
      maxMessageSize_(_settings["max_message_size"].asUInt()) {
  assert(minMessageSize_ > 0);
  assert(maxMessageSize_ > 0);
  assert(maxMessageSize_ >= minMessageSize_);
}

RandomMessageSizeDistribution::~RandomMessageSizeDistribution() {}

u32 RandomMessageSizeDistribution::minMessageSize() const {
  return minMessageSize_;
}

u32 RandomMessageSizeDistribution::maxMessageSize() const {
  return maxMessageSize_;
}

u32 RandomMessageSizeDistribution::nextMessageSize() {
  return gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
}

u32 RandomMessageSizeDistribution::nextMessageSize(const Message* _msg) {
  return nextMessageSize();
}
