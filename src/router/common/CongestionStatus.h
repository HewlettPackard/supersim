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
#ifndef ROUTER_COMMON_CONGESTIONSTATUS_H_
#define ROUTER_COMMON_CONGESTIONSTATUS_H_

#include <json/json.h>
#include <prim/prim.h>

#include <stack>
#include <string>
#include <vector>

#include "event/Component.h"

class CongestionStatus : public Component {
 public:
  CongestionStatus(const std::string& _name, const Component* _parent,
                   u32 _totalVcs, Json::Value _settings);
  ~CongestionStatus();

  // configuration
  void initCredits(u32 _vcIdx, u32 _credits);

  // operation
  void increment(u32 _vcIdx);
  void decrement(u32 _vcIdx);

  // this returns creditCount/maxCredits (buffer availability)
  f64 status(u32 _vcIdx) const;  // (must be epsilon >= 1)

  void processEvent(void* _event, s32 _type);

 private:
  void createEvent(u32 _vcIdx, s32 _type);

  const u32 totalVcs_;
  const u32 latency_;
  std::vector<u32> maximums_;
  std::vector<u32> counts_;
};

#endif  // ROUTER_COMMON_CONGESTIONSTATUS_H_
