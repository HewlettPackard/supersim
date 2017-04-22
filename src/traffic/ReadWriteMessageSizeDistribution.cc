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
#include "traffic/ReadWriteMessageSizeDistribution.h"

#include <factory/Factory.h>

#include <cassert>

#include <algorithm>

#include "event/Simulator.h"

ReadWriteMessageSizeDistribution::ReadWriteMessageSizeDistribution(
    const std::string& _name, const Component* _parent,
    Json::Value _settings)
    : MessageSizeDistribution(_name, _parent, _settings),
      readRequestSize_(_settings["read_request_size"].asUInt()),
      readResponseSize_(_settings["read_response_size"].asUInt()),
      writeRequestSize_(_settings["write_request_size"].asUInt()),
      writeResponseSize_(_settings["write_response_size"].asUInt()),
      readProbability_(_settings["read_probability"].asDouble()) {
  assert(readRequestSize_ > 0);
  assert(readResponseSize_ > 0);
  assert(writeRequestSize_ > 0);
  assert(writeResponseSize_ > 0);
  assert(!_settings["read_probability"].isNull());
  assert(readProbability_ >= 0.0 && readProbability_ <= 1.0);

  assert(readRequestSize_ != writeRequestSize_);

  if (readProbability_ == 0.0) {
    minMessageSize_ = std::min(writeRequestSize_, writeResponseSize_);
    maxMessageSize_ = std::max(writeRequestSize_, writeResponseSize_);
  } else if (readProbability_ == 1.0) {
    minMessageSize_ = std::min(readRequestSize_, readResponseSize_);
    maxMessageSize_ = std::max(readRequestSize_, readResponseSize_);
  } else {
    minMessageSize_ = std::min({readRequestSize_, readResponseSize_,
            writeRequestSize_, writeResponseSize_});
    maxMessageSize_ = std::max({readRequestSize_, readResponseSize_,
            writeRequestSize_, writeResponseSize_});
  }
}

ReadWriteMessageSizeDistribution::~ReadWriteMessageSizeDistribution() {}

u32 ReadWriteMessageSizeDistribution::minMessageSize() const {
  return minMessageSize_;
}

u32 ReadWriteMessageSizeDistribution::maxMessageSize() const {
  return maxMessageSize_;
}

u32 ReadWriteMessageSizeDistribution::nextMessageSize() {
  if (gSim->rnd.nextF64() <= readProbability_) {
    return readRequestSize_;
  } else {
    return writeRequestSize_;
  }
}

u32 ReadWriteMessageSizeDistribution::nextMessageSize(const Message* _msg) {
  if (_msg->numFlits() == readRequestSize_) {
    return readResponseSize_;
  } else if (_msg->numFlits() == writeRequestSize_) {
    return writeResponseSize_;
  } else {
    assert(false);
  }
}

registerWithFactory("read_write", MessageSizeDistribution,
                    ReadWriteMessageSizeDistribution,
                    MESSAGESIZEDISTRIBUTION_ARGS);
