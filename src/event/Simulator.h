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
#ifndef EVENT_SIMULATOR_H_
#define EVENT_SIMULATOR_H_

#include <json/json.h>
#include <prim/prim.h>
#include <rnd/Random.h>

class Component;
class Network;
class Application;

class Simulator {
 public:
  explicit Simulator(Json::Value _settings);
  virtual ~Simulator();

  // this adds an event to the queue
  virtual void addEvent(u64 _time, u8 _epsilon, Component* _component,
                        void* _event, s32 _type) = 0;

  // this function must return the current size of the queue
  virtual u64 queueSize() const = 0;

  void simulate();
  void stop();
  bool initial() const;
  bool running() const;
  u64 time() const;
  u8 epsilon() const;
  u64 cycle() const;
  u64 cycleTime() const;
  u64 futureCycle(u32 _cycles) const;
  void setNetwork(Network* _network);
  Network* getNetwork() const;
  void setApplication(Application* _app);
  Application* getApplication() const;
  bool getMonitoring() const;
  void startMonitoring();
  void endMonitoring();

  rnd::Random rnd;

 protected:
  const bool printProgress_;
  const f64 printInterval_;

  u64 time_;
  u8 epsilon_;
  bool quit_;

  // this function must set time_, epsilon_, and quit_ on every call
  virtual void runNextEvent() = 0;

 private:
  const u64 cycleTime_;

  bool initial_;
  bool running_;

  Network* net_;
  Application* app_;

  bool monitoring_;
};

extern Simulator* gSim;

#endif  // EVENT_SIMULATOR_H_
