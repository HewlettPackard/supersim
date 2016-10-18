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
#include "workload/RateMonitor.h"

RateMonitor::RateMonitor(const std::string& _name, const Component* _parent)
    : Component(_name, _parent),
      flitCount_(0), startTime_(U64_MAX), endTime_(U64_MAX), running_(false) {}

RateMonitor::~RateMonitor() {}

void RateMonitor::monitorMessage(const Message* _message) {
  if (running_) {
    flitCount_ += _message->numFlits();
  }
}

void RateMonitor::start() {
  if (!running_) {
    startTime_ = gSim->time();
    flitCount_ = 0;
  }
  running_ = true;
}

void RateMonitor::end() {
  if (running_) {
    endTime_ = gSim->time();
  }
  running_ = false;
}

f64 RateMonitor::rate() const {
  if ((startTime_ == U64_MAX) || (endTime_ == U64_MAX) || (running_)) {
    return F64_NAN;
  } else {
    f64 cycles = (f64)(endTime_ - startTime_) /
        gSim->cycleTime(Simulator::Clock::CHANNEL);
    return flitCount_ / cycles;
  }
}
