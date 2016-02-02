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
#include "application/ExitNotifier.h"

#include "application/Terminal.h"

ExitNotifier::ExitNotifier() {}

ExitNotifier::~ExitNotifier() {}

void ExitNotifier::setMessageReceiver(MessageReceiver* _receiver) {
  receiver_ = _receiver;
}

void ExitNotifier::receiveMessage(Message* _message) {
  // notify source of message exit
  _message->getOwner()->messageExitedNetwork(_message);

  // deliver to next recipient
  receiver_->receiveMessage(_message);
}
