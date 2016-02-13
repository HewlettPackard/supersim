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
#ifndef STATS_FILELOG_H_
#define STATS_FILELOG_H_

#include <json/json.h>
#include <zlib.h>

#include <string>

class FileLog {
 public:
  explicit FileLog(Json::Value _settings);
  virtual ~FileLog();

  void write(const std::string& _text);

 private:
  bool compress_;
  FILE* regFile_;
  gzFile gzFile_;
};

#endif  // STATS_FILELOG_H_
