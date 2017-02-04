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
#ifndef WORKLOAD_APPLICATION_H_
#define WORKLOAD_APPLICATION_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "stats/RateLog.h"

class MetadataHandler;
class Terminal;
class Workload;

class Application : public Component {
 public:
  Application(const std::string& _name, const Component* _parent, u32 _id,
              Workload* _workload, MetadataHandler* _metadataHandler,
              Json::Value _settings);
  virtual ~Application();
  u32 numTerminals() const;
  u32 id() const;
  Workload* workload() const;

  Terminal* getTerminal(u32 _id) const;
  MetadataHandler* metadataHandler() const;

  u64 createTransaction(u32 _termId, u32 _msgId);
  u64 transactionCreationTime(u64 _trans) const;
  void endTransaction(u64 _trans);

  void startMonitoring();
  void endMonitoring();

  // this provides a percentage of completion for status printing
  virtual f64 percentComplete() const = 0;

  // see workload/Workload.h about implementation of these functions
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void kill() = 0;

 protected:
  void setTerminal(u32 _id, Terminal* _terminal);

  u32 id_;
  Workload* workload_;

 private:
  std::vector<Terminal*> terminals_;
  RateLog* rateLog_;
  MetadataHandler* metadataHandler_;
  std::unordered_map<u64, u64> transactions_;
};

#endif  // WORKLOAD_APPLICATION_H_
