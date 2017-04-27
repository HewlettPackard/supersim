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

#include <factory/Factory.h>

#include <cassert>

#include "event/Simulator.h"

RandomMSD::RandomMSD(
    const std::string& _name, const Component* _parent,
    Json::Value _settings)
    : MessageSizeDistribution(_name, _parent, _settings),
      minMessageSize_(_settings["min_message_size"].asUInt()),
      maxMessageSize_(_settings["max_message_size"].asUInt()),
      doDependent_(_settings.isMember("dependent_min_message_size") &&
                   _settings.isMember("dependent_max_message_size")),
      depMinMessageSize_(_settings["dependent_min_message_size"].asUInt()),
      depMaxMessageSize_(_settings["dependent_max_message_size"].asUInt()) {
  assert(minMessageSize_ > 0);
  assert(maxMessageSize_ > 0);
  assert(maxMessageSize_ >= minMessageSize_);
  if (doDependent_) {
      assert(depMinMessageSize_ > 0);
      assert(depMaxMessageSize_ > 0);
      assert(depMaxMessageSize_ >= depMinMessageSize_);
  } else {
    assert(_settings["dependent_min_message_size"].isNull());
    assert(_settings["dependent_max_message_size"].isNull());
  }
}

RandomMSD::~RandomMSD() {}

u32 RandomMSD::minMessageSize() const {
  return minMessageSize_;
}

u32 RandomMSD::maxMessageSize() const {
  return maxMessageSize_;
}

u32 RandomMSD::nextMessageSize() {
  return gSim->rnd.nextU64(minMessageSize_, maxMessageSize_);
}

u32 RandomMSD::nextMessageSize(const Message* _msg) {
  if (doDependent_) {
    return gSim->rnd.nextU64(depMinMessageSize_, depMaxMessageSize_);
  } else {
    return nextMessageSize();
  }
}

registerWithFactory("random", MessageSizeDistribution,
                    RandomMSD,
                    MESSAGESIZEDISTRIBUTION_ARGS);
