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
#include "application/Application.h"

#include <cassert>
#include <cmath>

#include "application/Messenger.h"
#include "application/Terminal.h"
#include "network/Network.h"

Application::Application(const std::string& _name, const Component* _parent,
                         MetadataHandler* _metadataHandler,
                         Json::Value _settings)
    : Component(_name, _parent), metadataHandler_(_metadataHandler) {
  Network* network = gSim->getNetwork();

  u32 radix = network->numInterfaces();
  terminals_.resize(radix, nullptr);
  messengers_.resize(radix, nullptr);

  // extract maximum injection rate
  maxInjectionRate_ = _settings["max_injection_rate"].asDouble();
  if (maxInjectionRate_ <= 0.0) {
    maxInjectionRate_ = INFINITY;
  }

  // instantiate the messengers
  for (u32 id = 0; id < radix; id++) {
    // create the messenger
    std::string cname = "Messenger_" + std::to_string(id);
    messengers_.at(id) = new Messenger(cname, this, this, id);

    // link the messenger to the network
    messengers_.at(id)->linkInterface(network->getInterface(id));
  }

  // create a MessageLog
  messageLog_ = new MessageLog(_settings["message_log"]);

  // create the rate log
  rateLog_ = new RateLog(_settings["rate_log"]);
}

Application::~Application() {
  for (u32 idx = 0; idx < terminals_.size(); idx++) {
    delete terminals_.at(idx);
    delete messengers_.at(idx);
  }
  delete messageLog_;
  delete rateLog_;
}

u32 Application::numTerminals() const {
  return terminals_.size();
}

Terminal* Application::getTerminal(u32 _id) const {
  return terminals_.at(_id);
}

MessageLog* Application::getMessageLog() const {
  return messageLog_;
}

MetadataHandler* Application::getMetadataHandler() const {
  return metadataHandler_;
}

u64 Application::createTransaction(u32 _tid, u32 _lid) {
  u64 trans = ((u64)_tid << 32) | (u64)_lid;
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


u64 Application::cyclesToSend(u32 _numFlits) const {
  if (isinf(maxInjectionRate_)) {
    return 0;  // infinite injection rate
  }
  f64 cycles = _numFlits / maxInjectionRate_;

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
  for (u32 i = 0; i < messengers_.size(); i++) {
    messengers_.at(i)->startRateMonitors();
  }
}

void Application::endMonitoring() {
  for (u32 i = 0; i < messengers_.size(); i++) {
    messengers_.at(i)->endRateMonitors();
  }
  for (u32 i = 0; i < messengers_.size(); i++) {
    messengers_.at(i)->logRates(rateLog_);
  }
}

void Application::setTerminal(u32 _id, Terminal* _terminal) {
  terminals_.at(_id) = _terminal;
  messengers_.at(_id)->linkTerminal(terminals_.at(_id));
}
