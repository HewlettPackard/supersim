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
#ifndef TRAFFIC_CONTINUOUS_DRAGONFLYWORSTCASECTP_H_
#define TRAFFIC_CONTINUOUS_DRAGONFLYWORSTCASECTP_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "traffic/continuous/ContinuousTrafficPattern.h"

class DragonflyWorstCaseCTP : public ContinuousTrafficPattern {
 public:
  DragonflyWorstCaseCTP(
      const std::string& _name, const Component* _parent,
      u32 _numTerminals, u32 _self, Json::Value _settings);
  ~DragonflyWorstCaseCTP();

  u32 nextDestination() override;

 private:
  u32 groupCount_;
  u32 groupSize_;
  u32 concentration_;
  bool random_;

  u32 selfGroup_;
  u32 selfLocal_;
  u32 selfConc_;
  u32 destGroup_;
};

#endif  // TRAFFIC_CONTINUOUS_DRAGONFLYWORSTCASECTP_H_
