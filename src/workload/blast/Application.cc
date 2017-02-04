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
#include "workload/blast/Application.h"

#include <cassert>

#include <vector>

#include "workload/blast/BlastTerminal.h"
#include "event/Simulator.h"
#include "network/Network.h"


#define kForceWarmed   (0x123)
#define kMaxSaturation (0x456)

namespace Blast {

Application::Application(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings)
    : ::Application(_name, _parent, _id, _workload, _metadataHandler,
                    _settings),
      killOnSaturation_(_settings["kill_on_saturation"].asBool()),
      logDuringSaturation_(_settings["log_during_saturation"].asBool()),
      maxSaturationCycles_(_settings["max_saturation_cycles"].asUInt()),
      warmupThreshold_(_settings["warmup_threshold"].asDouble()) {
  // check settings
  assert(!_settings["kill_on_saturation"].isNull());
  assert(!_settings["log_during_saturation"].isNull());
  if (logDuringSaturation_) {
    assert(!_settings["max_saturation_cycles"].isNull());
  }
  assert(!_settings["warmup_threshold"].isNull());
  assert(warmupThreshold_ >= 0.0);
  assert(warmupThreshold_ <= 1.0);

  // all terminals are the same
  activeTerminals_ = numTerminals();
  for (u32 t = 0; t < numTerminals(); t++) {
    std::string tname = "BlastTerminal_" + std::to_string(t);
    std::vector<u32> address;
    gSim->getNetwork()->translateInterfaceIdToAddress(t, &address);
    BlastTerminal* terminal = new BlastTerminal(tname, this, t, address, this,
                                                _settings["blast_terminal"]);
    setTerminal(t, terminal);

    // remove terminals with no injection
    if (terminal->requestInjectionRate() == 0.0) {
      activeTerminals_--;
    }
  }

  // initialize state machine
  fsm_ = Application::Fsm::WARMING;

  // initialize counters
  warmedTerminals_ = 0;
  saturatedTerminals_ = 0;
  completedTerminals_ = 0;
  doneTerminals_ = 0;

  // force warmed if threshold is 0.0
  if (warmupThreshold_ == 0.0) {
    addEvent(0, 0, nullptr, kForceWarmed);
  }
}

Application::~Application() {}

f64 Application::percentComplete() const {
  f64 percentSum = 0.0;
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
    percentSum += t->percentComplete();
  }
  return percentSum / activeTerminals_;
}

void Application::start() {
  for (u32 idx = 0; idx < numTerminals(); idx++) {
    BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
    if (doLogging_) {
      t->startLogging();
    } else {
      t->stopSending();
    }
  }

  if (!doLogging_) {
    workload_->applicationComplete(id_);
  }
}

void Application::stop() {
  if (doLogging_) {
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->stopLogging();
    }
  } else {
    workload_->applicationDone(id_);
  }
}

void Application::kill() {
  if (doLogging_) {
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->stopSending();
    }
  }
}

void Application::terminalWarmed(u32 _id) {
  assert(fsm_ == Application::Fsm::WARMING);
  if (_id != U32_MAX) {
    warmedTerminals_++;
  }
  dbgprintf("Terminal %u is warmed (%u total)", _id, warmedTerminals_);
  assert(warmedTerminals_ <= activeTerminals_);
  f64 percentWarmed = warmedTerminals_ / static_cast<f64>(activeTerminals_);
  if (percentWarmed >= warmupThreshold_) {
    fsm_ = Application::Fsm::LOGGING;
    dbgprintf("Warmup threshold %f reached", warmupThreshold_);
    doLogging_ = true;
    for (u32 idx = 0; idx < numTerminals(); idx++) {
      BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
      t->stopWarming();
    }
    workload_->applicationReady(id_);
  }
}

void Application::terminalSaturated(u32 _id) {
  assert(fsm_ == Application::Fsm::WARMING);
  saturatedTerminals_++;
  dbgprintf("Terminal %u is saturated (%u total)", _id, saturatedTerminals_);
  assert(saturatedTerminals_ <= activeTerminals_);
  f64 percentSaturated = saturatedTerminals_ /
      static_cast<f64>(activeTerminals_);
  if (percentSaturated > (1.0 - warmupThreshold_)) {
    // the network is saturated
    if (killOnSaturation_) {
      // just kill the simulator right here
      dbgprintf("Saturation threshold %f reached, initiating kill fast",
                1.0 - warmupThreshold_);
      exit(0);
    } else if (logDuringSaturation_) {
      // start the logging phase
      dbgprintf("Saturation threshold %f reached, continuing anyway",
                1.0 - warmupThreshold_);
      fsm_ = Application::Fsm::LOGGING;
      doLogging_ = true;
      for (u32 idx = 0; idx < numTerminals(); idx++) {
        BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
        t->stopWarming();
      }
      workload_->applicationReady(id_);

      // set the maximum number of cycles to stay within the logging phase
      u64 timeout = gSim->futureCycle(Simulator::Clock::CHANNEL,
                                      maxSaturationCycles_);
      dbgprintf("setting timeout from %lu to %lu", gSim->time(), timeout);
      addEvent(timeout, 0, nullptr, kMaxSaturation);
    } else {
      // drain all the packets from the network
      dbgprintf("Saturation threshold %f reached",
                1.0 - warmupThreshold_);
      fsm_ = Application::Fsm::DRAINING;
      doLogging_ = false;
      for (u32 idx = 0; idx < numTerminals(); idx++) {
        BlastTerminal* t = reinterpret_cast<BlastTerminal*>(getTerminal(idx));
        t->stopWarming();
      }
      workload_->applicationReady(id_);
    }
  }
}

void Application::terminalComplete(u32 _id) {
  completedTerminals_++;
  dbgprintf("Terminal %u is done logging (%u total)",
            _id, completedTerminals_);
  assert(completedTerminals_ <= activeTerminals_);
  if ((completedTerminals_ == activeTerminals_) &&
      (fsm_ == Application::Fsm::LOGGING)) {
    dbgprintf("All terminals are done logging");
    fsm_ = Application::Fsm::BLABBING;
    workload_->applicationComplete(id_);
  }
}

void Application::terminalDone(u32 _id) {
  doneTerminals_++;
  dbgprintf("Terminal %u is done sending (%u total)", _id, doneTerminals_);
  assert(doneTerminals_ <= activeTerminals_);
  if (doneTerminals_ == activeTerminals_) {
    dbgprintf("All terminals are done sending");
    fsm_ = Application::Fsm::DRAINING;
    workload_->applicationDone(id_);
  }
}


void Application::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case kForceWarmed: {
      terminalWarmed(U32_MAX);
      break;
    }
    case kMaxSaturation: {
      if (fsm_ == Application::Fsm::LOGGING) {
        dbgprintf("Max saturation time reached");
        fsm_ = Application::Fsm::BLABBING;
        workload_->applicationComplete(id_);
      }
      break;
    }
    default:
      assert(false);
  }
}

}  // namespace Blast
