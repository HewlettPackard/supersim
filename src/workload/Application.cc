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
#include "workload/Application.h"

#include <factory/Factory.h>

#include <cassert>
#include <cmath>

#include "network/Network.h"
#include "workload/Terminal.h"
#include "workload/util.h"
#include "workload/Workload.h"

Application::Application(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings)
    : Component(_name, _parent), id_(_id), workload_(_workload),
      metadataHandler_(_metadataHandler) {
  Network* network = gSim->getNetwork();

  // application IDs can only consume 8 bits
  //  see createTransaction()
  assert(id_ < (1 << 8));

  u32 size = network->numInterfaces();
  // terminal IDs can only consume 24 bits
  //  see createTransaction()
  assert(size < (1 << 24));

  // make space for terminals
  terminals_.resize(size, nullptr);

  // create the rate log
  rateLog_ = new RateLog(_settings["rate_log"]);
}

Application::~Application() {
  for (u32 idx = 0; idx < terminals_.size(); idx++) {
    delete terminals_.at(idx);
  }
  delete rateLog_;
}

Application* Application::create(
    const std::string& _name, const Component* _parent, u32 _id,
    Workload* _workload, MetadataHandler* _metadataHandler,
    Json::Value _settings) {
  // retrieve the application type
  const std::string& type = _settings["type"].asString();

  // attempt to create the application
  Application* app = factory::Factory<Application, APPLICATION_ARGS>::create(
      type, _name, _parent, _id, _workload, _metadataHandler, _settings);

  // check that the factory has this type of application
  if (app == nullptr) {
    fprintf(stderr, "unknown application type: %s\n", type.c_str());
    assert(false);
  }
  return app;
}

u32 Application::numTerminals() const {
  return terminals_.size();
}

u32 Application::id() const {
  return id_;
}

Workload* Application::workload() const {
  return workload_;
}

Terminal* Application::getTerminal(u32 _id) const {
  return terminals_.at(_id);
}

MetadataHandler* Application::metadataHandler() const {
  return metadataHandler_;
}

u64 Application::createTransaction(u32 _termId, u32 _msgId) {
  u64 trans = transactionId(id_, _termId, _msgId);
  u64 now = gSim->time();
  bool res = transactions_.insert(std::make_pair(trans, now)).second;
  (void)res;  // unused
  assert(res);
  return trans;
}

u64 Application::transactionCreationTime(u64 _trans) const {
  return transactions_.at(_trans);
}

void Application::endTransaction(u64 _trans) {
  u64 res = transactions_.erase(_trans);
  (void)res;  // unused
  assert(res == 1);
}

void Application::startMonitoring() {
  for (u32 i = 0; i < terminals_.size(); i++) {
    terminals_.at(i)->startRateMonitors();
  }
}

void Application::endMonitoring() {
  for (u32 i = 0; i < terminals_.size(); i++) {
    terminals_.at(i)->endRateMonitors();
  }
  for (u32 i = 0; i < terminals_.size(); i++) {
    terminals_.at(i)->logRates(rateLog_);
  }
}

void Application::setTerminal(u32 _id, Terminal* _terminal) {
  terminals_.at(_id) = _terminal;
  _terminal->setMessageReceiver(gSim->getNetwork()->getInterface(_id));
  workload_->messageDistributor(_id)->setMessageReceiver(id_, _terminal);
}
