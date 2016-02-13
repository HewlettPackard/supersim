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
#ifndef STATS_CHANNELLOG_H_
#define STATS_CHANNELLOG_H_

#include <json/json.h>
#include <prim/prim.h>

#include <sstream>
#include <string>

#include "network/Channel.h"
#include "stats/FileLog.h"

class ChannelLog : public FileLog {
 public:
  explicit ChannelLog(Json::Value _settings);
  ~ChannelLog();
  void logChannel(const Channel* _channel);

 private:
  std::stringstream ss_;
};

#endif  // STATS_CHANNELLOG_H_
