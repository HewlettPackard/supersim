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
#include "application/Messenger.h"

#include "application/Application.h"
#include "application/InjectionLimiter.h"
#include "application/RateMonitor.h"
#include "application/ExitNotifier.h"
#include "application/EnterNotifier.h"
#include "application/Terminal.h"

#include "interface/Interface.h"
#include "types/MessageReceiver.h"

Messenger::Messenger(const std::string& _name, const Component* _parent,
                     Application* _app, u32 _id)
    : Component(_name, _parent), app_(_app), id_(_id) {
  // create the injection limiter
  injectionLimiter_ = new InjectionLimiter("InjectionLimiter", this, app_, _id);

  // create the rate monitors
  supplyMonitor_ = new RateMonitor("SupplyMonitor", this);
  injectionMonitor_ = new RateMonitor("InjectionMonitor", this);
  ejectionMonitor_ = new RateMonitor("EjectionMonitor", this);

  // create the notifiers
  exitNotifier_ = new ExitNotifier();
  enterNotifier_ = new EnterNotifier();

  // link up all components ready for linking
  supplyMonitor_->setMessageReceiver(injectionLimiter_);
  injectionLimiter_->setMessageReceiver(injectionMonitor_);
  injectionMonitor_->setMessageReceiver(enterNotifier_);

  ejectionMonitor_->setMessageReceiver(exitNotifier_);
}

Messenger::~Messenger() {
  delete injectionLimiter_;
  delete supplyMonitor_;
  delete injectionMonitor_;
  delete ejectionMonitor_;
  delete exitNotifier_;
  delete enterNotifier_;
}

void Messenger::linkInterface(Interface* _interface) {
  enterNotifier_->setMessageReceiver(_interface);
  _interface->setMessageReceiver(ejectionMonitor_);
}

void Messenger::linkTerminal(Terminal* _terminal) {
  _terminal->setMessageReceiver(supplyMonitor_);
  exitNotifier_->setMessageReceiver(_terminal);
}

void Messenger::startRateMonitors() {
  supplyMonitor_->start();
  injectionMonitor_->start();
  ejectionMonitor_->start();
}

void Messenger::endRateMonitors() {
  supplyMonitor_->end();
  injectionMonitor_->end();
  ejectionMonitor_->end();
}

void Messenger::logRates(RateLog* _rateLog) {
  _rateLog->logRates(id_, app_->getTerminal(id_)->name(),
                     supplyMonitor_->rate(), injectionMonitor_->rate(),
                     ejectionMonitor_->rate());
}
