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
#include "network/InjectionAlgorithm.h"

#include <cassert>

#include "event/Simulator.h"

/* InjectionAlgorithm::Response class */

InjectionAlgorithm::Response::Response() {}

InjectionAlgorithm::Response::~Response() {}

void InjectionAlgorithm::Response::clear() {
  response_.clear();
}

void InjectionAlgorithm::Response::add(u32 _vc) {
  response_.push_back(_vc);
}

u32 InjectionAlgorithm::Response::size() const {
  return response_.size();
}

void InjectionAlgorithm::Response::get(u32 _index, u32* _vc) const {
  const u32& val = response_.at(_index);
  *_vc = val;
}

/* InjectionAlgorithm::Client class */

InjectionAlgorithm::Client::Client() {}

InjectionAlgorithm::Client::~Client() {}

/* InjectionAlgorithm class */

InjectionAlgorithm::InjectionAlgorithm(
    const std::string& _name, const Component* _parent, Interface* _interface,
    u32 _latency)
    : Component(_name, _parent), interface_(_interface), latency_(_latency) {
  assert(latency_ > 0);
}

InjectionAlgorithm::~InjectionAlgorithm() {}

u32 InjectionAlgorithm::latency() const {
  return latency_;
}

void InjectionAlgorithm::request(Client* _client, Message* _message,
                                 Response* _response) {
  u64 respTime = gSim->futureCycle(latency_);
  EventPackage* evt = new EventPackage();
  evt->client = _client;
  evt->message = _message;
  evt->response = _response;
  addEvent(respTime, 0, evt, 0);
}

void InjectionAlgorithm::processEvent(void* _event, s32 _type) {
  EventPackage* evt = reinterpret_cast<EventPackage*>(_event);
  processRequest(evt->message, evt->response);
  evt->client->injectionAlgorithmResponse(evt->message, evt->response);
  delete evt;
}
