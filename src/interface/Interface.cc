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
#include "interface/Interface.h"

#include <cassert>

Interface::Interface(const std::string& _name, const Component* _parent,
                     u32 _id, Json::Value _settings)
    : Component(_name, _parent), id_(_id),
      numVcs_(_settings["num_vcs"].asUInt()) {
  assert(numVcs_ > 0);
}

Interface::~Interface() {}

u32 Interface::getId() const {
  return id_;
}

u32 Interface::numVcs() const {
  return numVcs_;
}

void Interface::setMessageReceiver(MessageReceiver* _receiver) {
  messageReceiver_ = _receiver;
}

MessageReceiver* Interface::getMessageReceiver() const {
  return messageReceiver_;
}
