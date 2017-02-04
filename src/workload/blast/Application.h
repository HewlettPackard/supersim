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
#ifndef WORKLOAD_BLAST_APPLICATION_H_
#define WORKLOAD_BLAST_APPLICATION_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "workload/Application.h"
#include "workload/Workload.h"

class MetadataHandler;

namespace Blast {

class Application : public ::Application {
 public:
  Application(const std::string& _name, const Component* _parent, u32 _id,
              Workload* _workload, MetadataHandler* _metadataHandler,
              Json::Value _settings);
  ~Application();
  f64 percentComplete() const override;
  void start() override;
  void stop() override;
  void kill() override;

  void terminalWarmed(u32 _id);
  void terminalSaturated(u32 _id);
  void terminalComplete(u32 _id);
  void terminalDone(u32 _id);

  void processEvent(void* _event, s32 _type) override;

 private:
  // WARMING = sending and monitoring outstanding flits to determine if warm/sat
  // LOGGING = sending messages marked to be logged
  // BLABBING = sending messages not marked to be logged
  // DRAINING = not sending messages
  enum class Fsm {WARMING, LOGGING, BLABBING, DRAINING};

  const bool killOnSaturation_;
  const bool logDuringSaturation_;
  const u64 maxSaturationCycles_;
  const f64 warmupThreshold_;

  Fsm fsm_;

  u32 activeTerminals_;
  u32 warmedTerminals_;
  u32 saturatedTerminals_;
  bool doLogging_;

  u32 completedTerminals_;
  u32 doneTerminals_;
};

}  // namespace Blast

#endif  // WORKLOAD_BLAST_APPLICATION_H_
