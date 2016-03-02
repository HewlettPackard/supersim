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
#include "router/inputqueued/Router.h"

#include <cassert>

#include "network/RoutingFunctionFactory.h"
#include "router/inputqueued/Ejector.h"
#include "router/inputqueued/InputQueue.h"

namespace InputQueued {

Router::Router(
    const std::string& _name, const Component* _parent,
    RoutingFunctionFactory* _routingFunctionFactory,
    Json::Value _settings)
    : ::Router(_name, _parent, _settings) {
  u32 inputQueueDepth = _settings["input_queue_depth"].asUInt();
  assert(inputQueueDepth > 0);

  // create the crossbar and schedulers
  crossbar_ = new Crossbar("Crossbar", this, numPorts_ * numVcs_, numPorts_,
                           _settings["crossbar"]);
  vcScheduler_ = new VcScheduler(
      "VcScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      _settings["vc_scheduler"]);
  crossbarScheduler_ = new CrossbarScheduler(
      "CrossbarScheduler", this, numPorts_ * numVcs_, numPorts_ * numVcs_,
      numPorts_, _settings["crossbar_scheduler"]);

  // create routing functions, input queues, link to routing function,
  //  crossbar, and schedulers
  routingFunctions_.resize(numPorts_ * numVcs_);
  inputQueues_.resize(numPorts_ * numVcs_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    for (u32 vc = 0; vc < numVcs_; vc++) {
      // initialize the credit count in the CrossbarScheduler
      crossbarScheduler_->initCreditCount(vcIndex(port, vc), inputQueueDepth);

      // create the name suffix
      std::string nameSuffix = "_" + std::to_string(port) + "_" +
                               std::to_string(vc);

      // routing function
      std::string rfname = "RoutingFunction" + nameSuffix;
      RoutingFunction* rf = _routingFunctionFactory->createRoutingFunction(
          rfname, this, this, port, _settings["routing"]);
      routingFunctions_.at(vcIndex(port, vc)) = rf;

      // compute the client index (same for VC alloc, SW alloc, and Xbar)
      u32 clientIndex = (port * numVcs_) + vc;

      // input queue
      std::string iqName = "InputQueue" + nameSuffix;
      InputQueue* iq = new InputQueue(
          iqName, this, this, inputQueueDepth, port, numVcs_, vc, rf,
          vcScheduler_, clientIndex, crossbarScheduler_, clientIndex, crossbar_,
          clientIndex);
      inputQueues_.at(vcIndex(port, vc)) = iq;

      // register the input queue with VC and crossbar schedulers
      vcScheduler_->setClient(clientIndex, iq);
      crossbarScheduler_->setClient(clientIndex, iq);
    }
  }

  // ejectors, link to crossbar
  ejectors_.resize(numPorts_, nullptr);
  for (u32 port = 0; port < numPorts_; port++) {
    std::string ejName = "Ejector_" + std::to_string(port);
    Ejector* ej = new Ejector(ejName, this, port);
    ejectors_.at(port) = ej;
    crossbar_->setReceiver(port, ej, 0);  // ejectors only have 1 port
  }

  // allocate slots for I/O channels
  inputChannels_.resize(numPorts_, nullptr);
  outputChannels_.resize(numPorts_, nullptr);
}

Router::~Router() {
  delete crossbar_;
  delete vcScheduler_;
  delete crossbarScheduler_;
  for (u32 vc = 0; vc < (numPorts_ * numVcs_); vc++) {
    delete routingFunctions_.at(vc);
    delete inputQueues_.at(vc);
  }
  for (u32 port = 0; port < numPorts_; port++) {
    delete ejectors_.at(port);
  }
}

void Router::setInputChannel(u32 _index, Channel* _channel) {
  inputChannels_.at(_index) = _channel;
  _channel->setSink(this, _index);
}

void Router::setOutputChannel(u32 _index, Channel* _channel) {
  outputChannels_.at(_index) = _channel;
  _channel->setSource(this, _index);
}

void Router::receiveFlit(u32 _port, Flit* _flit) {
  u32 vc = _flit->getVc();
  InputQueue* iq = inputQueues_.at(vcIndex(_port, vc));
  iq->receiveFlit(0, _flit);
}

void Router::receiveCredit(u32 _port, Credit* _credit) {
  while (_credit->more()) {
    u32 vc = _credit->getNum();
    crossbarScheduler_->incrementCreditCount(vcIndex(_port, vc));
  }
  delete _credit;
}

void Router::sendCredit(u32 _port, u32 _vc) {
  // ensure there is an outgoing credit for the next time slot
  Credit* credit = inputChannels_.at(_port)->getNextCredit();
  if (credit == nullptr) {
    credit = new Credit(numVcs_);
    inputChannels_.at(_port)->setNextCredit(credit);
  }

  // mark the credit with the specified VC
  credit->putNum(_vc);
}

void Router::sendFlit(u32 _port, Flit* _flit) {
  assert(outputChannels_.at(_port)->getNextFlit() == nullptr);
  outputChannels_.at(_port)->setNextFlit(_flit);
}

u32 Router::vcIndex(u32 _port, u32 _vc) const {
  return (_port * numVcs_) + _vc;
}

void Router::vcIndexRev(u32 _index, u32* _port, u32* _vc) const {
  assert(_index < (numPorts_ * numVcs_));
  *_port = _index / numVcs_;
  *_vc = _index % numVcs_;
}

}  // namespace InputQueued
