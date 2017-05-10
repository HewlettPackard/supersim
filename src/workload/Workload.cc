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
#include "workload/Workload.h"

#include <cassert>

#include "network/Network.h"
#include "workload/Application.h"

Workload::Workload(const std::string& _name, const Component* _parent,
                   MetadataHandler* _metadataHandler, Json::Value _settings)
    : Component(_name, _parent), fsm_(Workload::Fsm::READY),
      readyCount_(0), completeCount_(0), doneCount_(0), monitoring_(false) {
  // determine the number of applications in the workload
  assert(_settings.isMember("applications") &&
         _settings["applications"].isArray());
  u32 numApps = _settings["applications"].size();

  // create the message distributors
  u32 numInterfaces = gSim->getNetwork()->numInterfaces();
  distributors_.resize(numInterfaces, nullptr);
  for (u32 idx = 0; idx < numInterfaces; idx++) {
    distributors_.at(idx) = new MessageDistributor(
        "MessageDistributor_" + std::to_string(idx), this, numApps);
  }

  // link the message distributors to the interfaces of the network
  for (u32 idx = 0; idx < numInterfaces; idx++) {
    gSim->getNetwork()->getInterface(idx)->setMessageReceiver(
        distributors_.at(idx));
  }

  // create the applications
  applications_.resize(numApps, nullptr);
  for (u32 app = 0; app < numApps; app++) {
    applications_.at(app) = Application::create(
        "Application_" +  std::to_string(app), this, app, this,
        _metadataHandler, _settings["applications"][app]);
  }

  // create a MessageLog
  messageLog_ = new MessageLog(_settings["message_log"]);
}

Workload::~Workload() {
  for (auto app : applications_) {
    delete app;
  }
  for (auto dist : distributors_) {
    delete dist;
  }
  delete messageLog_;
}

u32 Workload::numApplications() const {
  return applications_.size();
}

Application* Workload::application(u32 _index) const {
  return applications_.at(_index);
}

MessageDistributor* Workload::messageDistributor(u32 _index) const {
  return distributors_.at(_index);
}

MessageLog* Workload::messageLog() const {
  return messageLog_;
}

void Workload::applicationReady(u32 _index) {
  dbgprintf("App %u is ready", _index);
  readyCount_++;
  assert(readyCount_ <= numApplications());

  if (readyCount_ == numApplications()) {
    assert(fsm_ == Workload::Fsm::READY);
    fsm_ = Workload::Fsm::COMPLETE;

    // signal applications to start
    // signal applications and network to start monitoring
    for (auto app : applications_) {
      app->start();
      app->startMonitoring();
    }
    monitoring_ = true;
    gSim->getNetwork()->startMonitoring();
  }
}

void Workload::applicationComplete(u32 _index) {
  dbgprintf("App %u is complete", _index);
  completeCount_++;
  assert(completeCount_ <= numApplications());

  if (completeCount_ == numApplications()) {
    assert(fsm_ == Workload::Fsm::COMPLETE);
    fsm_ = Workload::Fsm::DONE;

    // signal applications to stop
    for (auto app : applications_) {
      app->stop();
    }
  }
}

void Workload::applicationDone(u32 _index) {
  dbgprintf("App %u is done", _index);
  doneCount_++;
  assert(doneCount_ <= numApplications());

  if (doneCount_ == numApplications()) {
    assert(fsm_ == Workload::Fsm::DONE);
    fsm_ = Workload::Fsm::KILLED;

    // signal applications to stop
    // signal applications and network to end monitoring
    for (auto app : applications_) {
      app->kill();
      app->endMonitoring();
    }
    if (monitoring_) {
      monitoring_ = false;
      gSim->getNetwork()->endMonitoring();
    }
  }
}
