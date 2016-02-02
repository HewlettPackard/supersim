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
#include "stats/ChannelLog.h"

ChannelLog::ChannelLog(Json::Value _settings)
    : FileLog(_settings) {
  write("name,utilization\n");
  ss_.precision(6);
  ss_.setf(std::ios::fixed, std::ios::floatfield);
}

ChannelLog::~ChannelLog() {}

void ChannelLog::logChannel(const Channel* _channel) {
  ss_ << _channel->fullName() << ',' << _channel->utilization() << std::endl;
  write(ss_.str());
  ss_.str("");
  ss_.clear();
}
