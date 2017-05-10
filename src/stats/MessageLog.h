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
#ifndef STATS_MESSAGELOG_H_
#define STATS_MESSAGELOG_H_

#include <fio/OutFile.h>
#include <json/json.h>
#include <prim/prim.h>

#include "types/Message.h"

class MessageLog {
 public:
  explicit MessageLog(Json::Value _settings);
  ~MessageLog();
  void logMessage(const Message* _message);
  void startTransaction(u64 _trans);
  void endTransaction(u64 _trans);

 private:
  fio::OutFile outFile_;
};

#endif  // STATS_MESSAGELOG_H_
