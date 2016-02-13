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
#include "test/TestSetup_TEST.h"

#include <json/json.h>
#include <settings/settings.h>

#include <string>

#include "event/Component.h"
#include "event/Simulator.h"
#include "event/VectorQueue.h"

TestSetup::TestSetup(u64 _cycleTime, u64 _randomSeed) {
  std::string str =
      std::string("{\n") +
      "  \"simulator\": {\n" +
      "     \"cycle_time\": " + std::to_string(_cycleTime) + ",\n" +
      "     \"print_progress\": false,\n" +
      "     \"random_seed\": " + std::to_string(_randomSeed) + "\n" +
      "  }\n" +
      "}\n" +
      std::string();

  Json::Value settings;
  settings::initString(str.c_str(), &settings);

  gSim = new VectorQueue(settings["simulator"]);
}

TestSetup::~TestSetup() {
  delete gSim;
  gSim = nullptr;
  Component::clearNames();
}
