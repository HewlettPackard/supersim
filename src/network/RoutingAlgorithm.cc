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
#include "network/RoutingAlgorithm.h"

#include <cassert>

#include "event/Simulator.h"
#include "router/Router.h"

/* RoutingAlgorithm::Response class */

RoutingAlgorithm::Response::Response() {}

RoutingAlgorithm::Response::~Response() {}

void RoutingAlgorithm::Response::clear() {
  response_.clear();
}

void RoutingAlgorithm::Response::add(u32 _port, u32 _vc) {
  assert(_port < algorithm_->router_->numPorts());
  assert(_vc >= algorithm_->baseVc_);
  assert(_vc < algorithm_->baseVc_ + algorithm_->numVcs_);
  response_.push_back({_port, _vc});
}

u32 RoutingAlgorithm::Response::size() const {
  return response_.size();
}

void RoutingAlgorithm::Response::get(u32 _index, u32* _port, u32* _vc) const {
  const std::pair<u32, u32>& val = response_.at(_index);
  *_port = val.first;
  *_vc   = val.second;
}

void RoutingAlgorithm::Response::link(const RoutingAlgorithm* _algorithm) {
  algorithm_ = _algorithm;
}

/* RoutingAlgorithm::Client class */

RoutingAlgorithm::Client::Client() {}

RoutingAlgorithm::Client::~Client() {}

/* RoutingAlgorithm class */

RoutingAlgorithm::RoutingAlgorithm(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _baseVc, u32 _numVcs, u32 _inputPort, u32 _inputVc,
    Json::Value _settings)
    : Component(_name, _parent), router_(_router), baseVc_(_baseVc),
      numVcs_(_numVcs), inputPort_(_inputPort), inputVc_(_inputVc),
      latency_(_settings["latency"].asUInt()) {
  assert(router_ != nullptr);
  assert(latency_ > 0);
  assert(numVcs_ <= router_->numVcs());
  assert(baseVc_ <= router_->numVcs() - numVcs_);
}

RoutingAlgorithm::~RoutingAlgorithm() {}

u32 RoutingAlgorithm::latency() const {
  return latency_;
}

u32 RoutingAlgorithm::baseVc() const {
  return baseVc_;
}

u32 RoutingAlgorithm::numVcs() const {
  return numVcs_;
}

u32 RoutingAlgorithm::inputPort() const {
  return inputPort_;
}

u32 RoutingAlgorithm::inputVc() const {
  return inputVc_;
}

void RoutingAlgorithm::request(Client* _client, Flit* _flit,
                               Response* _response) {
  u64 respTime = gSim->futureCycle(Simulator::Clock::CORE, latency_);
  EventPackage* evt = new EventPackage();
  evt->client = _client;
  evt->flit = _flit;
  evt->response = _response;
  addEvent(respTime, 0, evt, 0);
}

void RoutingAlgorithm::vcScheduled(Flit* _flit, u32 _port, u32 _vc) {}

void RoutingAlgorithm::processEvent(void* _event, s32 _type) {
  EventPackage* evt = reinterpret_cast<EventPackage*>(_event);
  processRequest(evt->flit, evt->response);
  evt->client->routingAlgorithmResponse(evt->response);
  delete evt;
}
