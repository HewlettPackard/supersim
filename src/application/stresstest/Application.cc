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
#include "application/stresstest/Application.h"

#include <cassert>

#include <vector>

#include "application/stresstest/BlastTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"

namespace StressTest {

Application::Application(const std::string& _name, const Component* _parent,
                         MetadataHandler* _metadataHandler,
                         Json::Value _settings)
    : ::Application(_name, _parent, _metadataHandler, _settings),
      killFast_(_settings["kill_fast"].asBool()) {
  assert(!_settings["kill_fast"].isNull());

  // all terminals are the same
  for (u32 t = 0; t < numTerminals(); t++) {
    std::string tname = "BlastTerminal_" + std::to_string(t);
    std::vector<u32> address;
    gSim->getNetwork()->translateTerminalIdToAddress(t, &address);
    BlastTerminal* terminal = new BlastTerminal(tname, this, t, address, this,
                                                _settings["blast_terminal"]);
    setTerminal(t, terminal);
  }

  warmedTerminals_ = 0;
  saturatedTerminals_ = 0;
  warmupComplete_ = false;
  completedTerminals_ = 0;
  warmupThreshold_ = _settings["warmup_threshold"].asDouble();
  assert(warmupThreshold_ > 0);
  assert(warmupThreshold_ <= 1.0);
}

Application::~Application() {}

void Application::terminalWarmed(u32 _id) {
  assert(warmupComplete_ == false);
  warmedTerminals_++;
  dbgprintf("Terminal %u is warmed (%u total)", _id, warmedTerminals_);
  assert(warmedTerminals_ <= numTerminals());
  f64 percentWarmed = warmedTerminals_ / static_cast<f64>(numTerminals());
  if (percentWarmed >= warmupThreshold_) {
    warmupComplete_ = true;
    printf("Warmup threshold %f reached, starting logging phase\n",
           warmupThreshold_);
    gSim->startMonitoring();
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->startLogging();
    }
  }
}

void Application::terminalSaturated(u32 _id) {
  assert(warmupComplete_ == false);
  saturatedTerminals_++;
  dbgprintf("Terminal %u is saturated (%u total)", _id, saturatedTerminals_);
  assert(saturatedTerminals_ <= numTerminals());
  f64 percentSaturated = saturatedTerminals_ / static_cast<f64>(numTerminals());
  if (percentSaturated > (1.0 - warmupThreshold_)) {
    warmupComplete_ = true;
    if (killFast_) {
      printf("Saturation threshold %f reached, initiating kill fast\n",
             1.0 - warmupThreshold_);
      exit(0);
    }
    printf("Saturation threshold %f reached, now draining the network\n",
           1.0 - warmupThreshold_);
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->stopSending();
    }
  }
}

void Application::terminalComplete(u32 _id) {
  completedTerminals_++;
  dbgprintf("Terminal %u is done (%u total)", _id, completedTerminals_);
  assert(completedTerminals_ <= numTerminals());
  if (completedTerminals_ == numTerminals()) {
    printf("All terminals have completed, now draining the network\n");
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->stopSending();
    }
    gSim->endMonitoring();
  }
}

f64 Application::percentComplete() const {
  f64 percentSum = 0.0;
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
    percentSum += t->percentComplete();
  }
  return percentSum / numTerminals();
}

}  // namespace StressTest
