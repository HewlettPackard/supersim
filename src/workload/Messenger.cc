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
#include "workload/Messenger.h"

#include "network/Network.h"
#include "workload/Application.h"
#include "workload/InjectionLimiter.h"
#include "workload/RateMonitor.h"
#include "workload/ExitNotifier.h"
#include "workload/EnterNotifier.h"
#include "workload/Terminal.h"
#include "workload/Workload.h"

#include "interface/Interface.h"
#include "types/MessageReceiver.h"

Messenger::Messenger(const std::string& _name, const Component* _parent,
                     Application* _app, u32 _id)
    : Component(_name, _parent), app_(_app), id_(_id) {
  // create the injection limiter
  injectionLimiter_ = new InjectionLimiter("InjectionLimiter", this, app_, id_);

  // create the notifiers
  exitNotifier_ = new ExitNotifier();
  enterNotifier_ = new EnterNotifier();

  // link the injection limiter to the enter notifier
  injectionLimiter_->setMessageReceiver(enterNotifier_);

  // link the enter notifier to the network interface
  enterNotifier_->setMessageReceiver(gSim->getNetwork()->getInterface(id_));

  // link the message distributor to the exit notifier
  app_->workload()->messageDistributor(id_)->setMessageReceiver(
      app_->id(), exitNotifier_);
}

Messenger::~Messenger() {
  delete injectionLimiter_;
  delete exitNotifier_;
  delete enterNotifier_;
}

void Messenger::linkTerminal(Terminal* _terminal) {
  _terminal->setMessageReceiver(injectionLimiter_);
  exitNotifier_->setMessageReceiver(_terminal);
}
