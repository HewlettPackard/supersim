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
#ifndef EVENT_SIMULATOR_H_
#define EVENT_SIMULATOR_H_

#include <json/json.h>
#include <prim/prim.h>
#include <rnd/Random.h>

#include "stats/InfoLog.h"

class Component;
class Network;
class Workload;

class Simulator {
 public:
  explicit Simulator(Json::Value _settings);
  virtual ~Simulator();

  // this adds an event to the queue
  virtual void addEvent(u64 _time, u8 _epsilon, Component* _component,
                        void* _event, s32 _type) = 0;

  // this function must return the current size of the queue
  virtual u64 queueSize() const = 0;

  void initialize();
  void simulate();
  void stop();
  bool initial() const;
  bool running() const;

  u64 time() const;
  u8 epsilon() const;

  enum class Clock : u8 {CHANNEL = 0, ROUTER = 1, INTERFACE = 2};

  u64 cycleTime(Clock _clock) const;
  u64 cycle(Clock _clock) const;
  bool isCycle(Clock _clock) const;
  u64 futureCycle(Clock _clock, u32 _cycles) const;

  void setNetwork(Network* _network);
  Network* getNetwork() const;
  void setWorkload(Workload* _workload);
  Workload* getWorkload() const;

  rnd::Random rnd;
  InfoLog infoLog;

 protected:
  // this function must set time_, epsilon_, and quit_ on every call
  virtual void runNextEvent() = 0;

  const bool printProgress_;
  const f64 printInterval_;

  u64 time_;
  u8 epsilon_;
  bool quit_;

 private:
  const u64 channelCycleTime_;
  const u64 routerCycleTime_;
  const u64 interfaceCycleTime_;

  bool initial_;
  bool initialized_;
  bool running_;

  Network* net_;
  Workload* workload_;
};

extern Simulator* gSim;

#endif  // EVENT_SIMULATOR_H_
