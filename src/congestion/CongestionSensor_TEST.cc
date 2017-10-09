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
#include "congestion/CongestionSensor_TEST.h"

#include <gtest/gtest.h>

#include <queue>
#include <tuple>

#include "event/Component.h"
#include "test/TestSetup_TEST.h"

/*********************** CongestionTestRouter class ***************************/

CongestionTestRouter::CongestionTestRouter(
    const std::string& _name, const Component* _parent, Network* _network,
    u32 _id, const std::vector<u32>& _address, u32 _numPorts, u32 _numVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings)
    : Router(_name, _parent, _network, _id, _address, _numPorts, _numVcs,
             _metadataHandler, _settings),
      congestionSensor_(nullptr) {
  outputChannels_.resize(numPorts_);
      }

CongestionTestRouter::~CongestionTestRouter() {}

void CongestionTestRouter::setCongestionSensor(
    CongestionSensor* _congestionSensor) {
  congestionSensor_ = _congestionSensor;
}

void CongestionTestRouter::setInputChannel(u32 _port, Channel* _channel) {
  assert(false);
}

Channel* CongestionTestRouter::getInputChannel(u32 _port) const {
  assert(false);
}

void CongestionTestRouter::setOutputChannel(u32 _port, Channel* _channel) {
  outputChannels_.at(_port) = _channel;
}

Channel* CongestionTestRouter::getOutputChannel(u32 _port) const {
  return outputChannels_.at(_port);
}

void CongestionTestRouter::receiveFlit(u32 _port, Flit* _flit) {
  assert(false);
}

void CongestionTestRouter::receiveCredit(u32 _port, Credit* _credit) {
  assert(false);
}

void CongestionTestRouter::sendCredit(u32 _port, u32 _vc) {
  assert(false);
}

void CongestionTestRouter::sendFlit(u32 _port, Flit* _flit) {
  assert(false);
}

f64 CongestionTestRouter::congestionStatus(
    u32 _inputPort, u32 _inputVc, u32 _outputPort, u32 _outputVc) const {
  return congestionSensor_->status(_inputPort, _inputVc,
                                   _outputPort, _outputVc);
}

/************************* CreditHandler utility class ************************/

CreditHandler::CreditHandler(
    const std::string& _name, const Component* _parent,
    CongestionSensor* _congestionSensor, PortedDevice* _device)
    : Component(_name, _parent), congestionSensor_(_congestionSensor),
      device_(_device) {}

CreditHandler::~CreditHandler() {}

void CreditHandler::setEvent(u32 _port, u32 _vc, u64 _time, u8 _epsilon,
                             CreditHandler::Type _type) {
  CreditHandler::Event* evt = new CreditHandler::Event({_type, _port, _vc});
  addEvent(_time, _epsilon, evt, 0);
}

void CreditHandler::processEvent(void* _event, s32 _type) {
  Event* evt = reinterpret_cast<Event*>(_event);
  u32 vcIdx = device_->vcIndex(evt->port, evt->vc);
  switch (evt->type) {
    case CreditHandler::Type::INCR:
      dbgprintf("incrementing port=%u vc=%u", evt->port, evt->vc);
      congestionSensor_->incrementCredit(vcIdx);
      break;

    case CreditHandler::Type::DECR:
      dbgprintf("decrementing port=%u vc=%u", evt->port, evt->vc);
      congestionSensor_->decrementCredit(vcIdx);
      break;

    default:
      assert(false);
  }
  delete evt;
}

/************************* StatusCheck utility class **************************/

StatusCheck::StatusCheck(const std::string& _name, const Component* _parent,
                         CongestionSensor* _congestionSensor)
    : Component(_name, _parent), congestionSensor_(_congestionSensor) {}

StatusCheck::~StatusCheck() {}

void StatusCheck::setEvent(u64 _time, u8 _epsilon, u32 _inputPort, u32 _inputVc,
                           u32 _outputPort, u32 _outputVc, f64 _expected) {
  StatusCheck::Event* evt = new StatusCheck::Event(
      {_inputPort, _inputVc, _outputPort, _outputVc, _expected});
  addEvent(_time, _epsilon, evt, 0);
}

void StatusCheck::processEvent(void* _event, s32 _type) {
  Event* evt = reinterpret_cast<Event*>(_event);
  f64 sts = congestionSensor_->status(evt->inputPort, evt->inputVc,
                                      evt->outputPort, evt->outputVc);
  if (sts - evt->exp > 0.002) {
    printf("sts=%f exp=%f\n", sts, evt->exp);
  }
  ASSERT_NEAR(sts, evt->exp, 0.002);
  delete evt;
}
