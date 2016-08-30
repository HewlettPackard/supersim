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
#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "stats/MessageLog.h"
#include "stats/RateLog.h"

class Messenger;
class MetadataHandler;
class Terminal;

class Application : public Component {
 public:
  Application(const std::string& _name, const Component* _parent,
              MetadataHandler* _metadataHandler, Json::Value _settings);
  ~Application();
  u32 numTerminals() const;

  Terminal* getTerminal(u32 _id) const;
  MessageLog* getMessageLog() const;
  MetadataHandler* getMetadataHandler() const;

  u64 createTransaction(u32 _tid, u32 _lid);
  u64 transactionCreationTime(u64 _trans) const;
  void endTransaction(u64 _trans);

  u64 cyclesToSend(u32 _numFlits, f64 _maxInjectionRate) const;

  void startMonitoring();
  void endMonitoring();

  virtual f64 percentComplete() const = 0;

 protected:
  void setTerminal(u32 _id, Terminal* _terminal);

 private:
  std::vector<Terminal*> terminals_;
  std::vector<Messenger*> messengers_;
  MessageLog* messageLog_;
  RateLog* rateLog_;
  MetadataHandler* metadataHandler_;
  std::unordered_map<u64, u64> transactions_;
};

#endif  // APPLICATION_APPLICATION_H_
