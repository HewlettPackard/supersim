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
#include "workload/Application.h"

#include <fio/InFile.h>
#include <strop/strop.h>

#include <cassert>
#include <cmath>

#include "workload/Messenger.h"
#include "workload/Terminal.h"
#include "workload/util.h"
#include "network/Network.h"

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

  // make space for terminals and messengers
  terminals_.resize(size, nullptr);
  messengers_.resize(size, nullptr);

  // extract maximum injection rate per terminal
  maxInjectionRates_.resize(size, F64_NAN);

  // get the base injection rate
  assert(_settings.isMember("max_injection_rate"));
  f64 baseInjection = _settings["max_injection_rate"].asDouble();
  if (baseInjection < 0.0) {
    baseInjection = INFINITY;
  }
  for (f64& mir : maxInjectionRates_) {
    mir = baseInjection;
  }

  // if necessary, apply relative rates
  if (_settings.isMember("relative_injection")) {
    // if a file is given, it is a csv of injection rates
    fio::InFile inf(_settings["relative_injection"].asString());
    std::string line;
    u32 lineNum = 0;
    fio::InFile::Status sts = fio::InFile::Status::OK;
    for (lineNum = 0; sts == fio::InFile::Status::OK;) {
      sts = inf.getLine(&line);
      assert(sts != fio::InFile::Status::ERROR);
      if (sts == fio::InFile::Status::OK) {
        if (line.size() > 0) {
          std::vector<std::string> strs = strop::split(line, ',');
          assert(strs.size() == 1);
          f64 ri = std::stod(strs.at(0));
          assert(ri >= 0.0);
          maxInjectionRates_.at(lineNum) *= ri;
          lineNum++;
        }
      }
    }
    assert(lineNum == size);
  }

  // instantiate the messengers
  for (u32 id = 0; id < size; id++) {
    // create the messenger
    std::string cname = "Messenger_" + std::to_string(id);
    messengers_.at(id) = new Messenger(cname, this, this, id);
  }

  // create the rate log
  rateLog_ = new RateLog(_settings["rate_log"]);
}

Application::~Application() {
  for (u32 idx = 0; idx < terminals_.size(); idx++) {
    delete terminals_.at(idx);
    delete messengers_.at(idx);
  }
  delete rateLog_;
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

f64 Application::maxInjectionRate(u32 _id) const {
  return maxInjectionRates_.at(_id);
}

u64 Application::cyclesToSend(u32 _id, u32 _numFlits) const {
  f64 maxInjectionRate = maxInjectionRates_.at(_id);
  if (std::isinf(maxInjectionRate)) {
    return 0;  // infinite injection rate
  }
  assert(maxInjectionRate > 0.0);
  f64 cycles = _numFlits / maxInjectionRate;

  // if the number of cycles is not even, probablistic cycles must be computed
  f64 fraction = modf(cycles, &cycles);
  if (fraction != 0.0) {
    assert(fraction > 0.0);
    assert(fraction < 1.0);
    f64 rnd = gSim->rnd.nextF64();
    if (fraction > rnd) {
      cycles += 1.0;
    }
  }
  return (u64)cycles;
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
  messengers_.at(_id)->linkTerminal(terminals_.at(_id));
}
