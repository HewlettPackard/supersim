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
#ifndef APPLICATION_STRESSTEST_APPLICATION_H_
#define APPLICATION_STRESSTEST_APPLICATION_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "application/Application.h"

class MetadataHandler;

namespace StressTest {

class Application : public ::Application {
 public:
  Application(const std::string& _name, const Component* _parent,
              MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Application();
  void terminalWarmed(u32 _id);
  void terminalSaturated(u32 _id);
  void terminalComplete(u32 _id);
  f64 percentComplete() const override;

  void processEvent(void* _event, s32 _type) override;

 private:
  const bool killOnSaturation_;
  const bool logDuringSaturation_;
  const u64 maxSaturationCycles_;

  u32 warmedTerminals_;
  u32 saturatedTerminals_;
  bool warmupComplete_;
  bool logComplete_;
  f64 warmupThreshold_;
  u32 completedTerminals_;
};

}  // namespace StressTest

#endif  // APPLICATION_STRESSTEST_APPLICATION_H_
