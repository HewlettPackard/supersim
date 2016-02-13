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
#ifndef STATS_RATELOG_H_
#define STATS_RATELOG_H_

#include <json/json.h>
#include <prim/prim.h>

#include <sstream>
#include <string>

#include "stats/FileLog.h"

class RateLog : public FileLog {
 public:
  explicit RateLog(Json::Value _settings);
  ~RateLog();
  void logRates(u32 _terminalId, const std::string& _terminalName,
                f64 _supplyRate, f64 _injectionRate, f64 _ejectionRate);

 private:
  std::stringstream ss_;
};

#endif  // STATS_RATELOG_H_
