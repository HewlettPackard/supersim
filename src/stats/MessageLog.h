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
#ifndef STATS_MESSAGELOG_H_
#define STATS_MESSAGELOG_H_

#include <jsoncpp/json/json.h>

#include "stats/FileLog.h"
#include "types/Message.h"

class MessageLog : public FileLog {
 public:
  explicit MessageLog(Json::Value _settings);
  ~MessageLog();
  void logMessage(const Message* _message);
};

#endif  // STATS_MESSAGELOG_H_
