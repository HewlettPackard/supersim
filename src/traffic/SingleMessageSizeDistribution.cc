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
#include "traffic/SingleMessageSizeDistribution.h"

#include <factory/Factory.h>

#include <cassert>

#include "event/Simulator.h"

SingleMessageSizeDistribution::SingleMessageSizeDistribution(
    const std::string& _name, const Component* _parent,
    Json::Value _settings)
    : MessageSizeDistribution(_name, _parent, _settings),
      messageSize_(_settings["message_size"].asUInt()),
      doDependent_(_settings.isMember("dependent_message_size")),
      depMessageSize_(_settings["dependent_message_size"].asUInt()) {
  assert(messageSize_ > 0);
  if (doDependent_) {
    assert(depMessageSize_ > 0);
  }
}

SingleMessageSizeDistribution::~SingleMessageSizeDistribution() {}

u32 SingleMessageSizeDistribution::minMessageSize() const {
  return messageSize_;
}

u32 SingleMessageSizeDistribution::maxMessageSize() const {
  return messageSize_;
}

u32 SingleMessageSizeDistribution::nextMessageSize() {
  return messageSize_;
}

u32 SingleMessageSizeDistribution::nextMessageSize(const Message* _msg) {
  if (doDependent_) {
    return depMessageSize_;
  } else {
    return nextMessageSize();
  }
}

registerWithFactory("single", MessageSizeDistribution,
                    SingleMessageSizeDistribution,
                    MESSAGESIZEDISTRIBUTION_ARGS);
