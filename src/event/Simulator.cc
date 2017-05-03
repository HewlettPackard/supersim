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
#include "event/Simulator.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <chrono>

#include "network/Network.h"
#include "workload/Application.h"
#include "workload/Workload.h"

Simulator::Simulator(Json::Value _settings)
    : printProgress_(_settings["print_progress"].asBool()),
      printInterval_(_settings["print_interval"].asDouble()),
      time_(0), epsilon_(0), quit_(false),
      channelCycleTime_(_settings["channel_cycle_time"].asUInt64()),
      coreCycleTime_(_settings["core_cycle_time"].asUInt64()),
      initial_(true), running_(false), net_(nullptr), workload_(nullptr) {
  assert(!_settings["print_progress"].isNull());
  assert(!_settings["print_interval"].isNull());
  assert(!_settings["channel_cycle_time"].isNull());
  assert(!_settings["core_cycle_time"].isNull());
  assert(!_settings["random_seed"].isNull());
  assert(channelCycleTime_ > 0);
  assert(coreCycleTime_ > 0);
  assert(printInterval_ > 0);

  rnd.seed(_settings["random_seed"].asUInt64());
}

Simulator::~Simulator() {}

void Simulator::simulate() {
  assert(running_ == false);
  initial_ = false;
  running_ = true;

  u64 totalEvents = 0;
  u64 intervalEvents = 0;

  u64 lastSimTime = 0;
  std::chrono::steady_clock::time_point startTime =
      std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point lastRealTime = startTime;

  std::chrono::duration<f64> sum(0);

  while (true) {
    if (quit_) {
      std::chrono::steady_clock::time_point realTime =
          std::chrono::steady_clock::now();
      std::chrono::duration<f64> totalElapsedRealTime =
          std::chrono::duration_cast<std::chrono::duration<f64> >
          (realTime - startTime);
      f64 runTime = totalElapsedRealTime.count();
      f64 ftime = static_cast<f64>(time_);

      if (printProgress_) {
        f64 eventsPerSecond = totalEvents / runTime;
        f64 eventsPerUnit = totalEvents / ftime;
        f64 unitsPerSecond = ftime / runTime;
        printf("\n"
               "Total event count:      %lu\n"
               "Total simulation units: %lu\n"
               "Total real seconds:     %.3f\n"
               "\n"
               "Events per real seconds:    %.3f\n"
               "Events per sim units:       %.3f\n"
               "Sim units per real seconds: %.3f\n"
               "\n",
               totalEvents, time_, runTime, eventsPerSecond, eventsPerUnit,
               unitsPerSecond);
      }
      break;
    } else {
      // tell the queue implemention to run the next event
      runNextEvent();

      // do timing calculations
      totalEvents++;
      intervalEvents++;

      if (printProgress_ && ((intervalEvents & 0xff) == 0)) {
        u64 elapsedSimTime = time_ - lastSimTime;
        std::chrono::steady_clock::time_point realTime =
            std::chrono::steady_clock::now();
        f64 elapsedRealTime =
            std::chrono::duration_cast<std::chrono::duration<f64> >
            (realTime - lastRealTime).count();

        if (elapsedRealTime > printInterval_) {
          lastSimTime = time_;

          // use a common buffer and snprintf
          char buf[256];  // much larger than needed
          u32 cnt = 0;

          // compute the human readable time
          f64 totalRealTime =
              std::chrono::duration_cast<std::chrono::duration<f64> >
              (realTime - startTime).count();
          u64 milliseconds = static_cast<u32>(totalRealTime * 1000);
          const u64 dayFactor = 24 * 60 * 60 * 1000;
          u64 days = milliseconds / dayFactor;
          milliseconds %= dayFactor;
          const u64 hourFactor = 60 * 60 * 1000;
          u64 hours = milliseconds / hourFactor;
          milliseconds %= hourFactor;
          const u64 minuteFactor = 60 * 1000;
          u64 minutes = milliseconds / minuteFactor;
          milliseconds %= minuteFactor;
          const u64 secondFactor = 1000;
          u64 seconds = milliseconds / secondFactor;
          milliseconds %= secondFactor;

          // print the time
          cnt = snprintf(buf + cnt, 256 - cnt, "%lu:%02lu:%02lu:%02lu [",
                         days, hours, minutes, seconds);

          // print the applications' progress
          u32 numApps = workload_->numApplications();
          for (u32 appId = 0; appId < numApps; appId++) {
            Application* app = workload_->application(appId);
            u64 perc = (u64)(app->percentComplete() * 100);
            if (appId < numApps - 1) {
              cnt += snprintf(buf + cnt, 256 - cnt, "%lu%%,", perc);
            } else {
              cnt += snprintf(buf + cnt, 256 - cnt, "%lu%%] ", perc);
            }
          }

          // print the simulation performance
          f64 eventsPerSecond = intervalEvents / elapsedRealTime;
          f64 unitsPerSecond = elapsedSimTime /
              static_cast<f64>(elapsedRealTime);
          snprintf(buf + cnt, 256 - cnt, "%lu events : %lu units : "
                   "%.2f events/sec : %.2f units/sec\n", totalEvents, time_,
                   eventsPerSecond, unitsPerSecond);

          // now print the entire buffer to stdout
          printf("%s", buf);

          lastRealTime = realTime;
          intervalEvents = 0;
        }
      }
    }
  }
  running_ = false;
  quit_ = false;
}

void Simulator::stop() {
  quit_ = true;
}

bool Simulator::initial() const {
  return initial_;
}

bool Simulator::running() const {
  return running_;
}

u64 Simulator::time() const {
  return time_;
}

u8 Simulator::epsilon() const {
  return epsilon_;
}

u64 Simulator::cycleTime(Simulator::Clock _clock) const {
  return _clock == Simulator::Clock::CHANNEL ?
      channelCycleTime_ : coreCycleTime_;
}

u64 Simulator::cycle(Simulator::Clock _clock) const {
  return time_ / cycleTime(_clock);
}

u64 Simulator::futureCycle(Simulator::Clock _clock, u32 _cycles) const {
  assert(_cycles > 0);

  u64 cycleTimeV = cycleTime(_clock);
  u64 time = gSim->time();
  time = time / cycleTimeV;
  time = (time + (u64)_cycles) * cycleTimeV;
  return time;
}

void Simulator::setNetwork(Network* _network) {
  net_ = _network;
}

Network* Simulator::getNetwork() const {
  return net_;
}

void Simulator::setWorkload(Workload* _workload) {
  workload_ = _workload;
}

Workload* Simulator::getWorkload() const {
  return workload_;
}


/* globals */
Simulator* gSim;
