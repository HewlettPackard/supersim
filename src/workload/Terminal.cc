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
#include "workload/Terminal.h"

#include <cassert>

#include "workload/Application.h"
#include "event/Simulator.h"
#include "metadata/MetadataHandler.h"

Terminal::Terminal(const std::string& _name, const Component* _parent, u32 _id,
                   const std::vector<u32>& _address, Application* _app)
    : Component(_name, _parent), id_(_id), address_(_address), app_(_app),
      messagesSent_(0), messagesReceived_(0), transactionsCreated_(0) {
  // create the rate monitors
  supplyMonitor_ = new RateMonitor("SupplyMonitor", this);
  injectionMonitor_ = new RateMonitor("InjectionMonitor", this);
  deliveredMonitor_ = new RateMonitor("DeliveredMonitor", this);
  ejectionMonitor_ = new RateMonitor("EjectionMonitor", this);
}

Terminal::~Terminal() {
  for (auto it = outstandingMessages_.begin(); it != outstandingMessages_.end();
       ++it) {
    delete *it;
  }
  delete supplyMonitor_;
  delete injectionMonitor_;
  delete deliveredMonitor_;
  delete ejectionMonitor_;
}

u32 Terminal::id() const {
  return id_;
}

const std::vector<u32>& Terminal::address() const {
  return address_;
}

Application* Terminal::application() const {
  return app_;
}

void Terminal::startRateMonitors() {
  supplyMonitor_->start();
  injectionMonitor_->start();
  deliveredMonitor_->start();
  ejectionMonitor_->start();
}

void Terminal::endRateMonitors() {
  supplyMonitor_->end();
  injectionMonitor_->end();
  deliveredMonitor_->end();
  ejectionMonitor_->end();
}

void Terminal::logRates(RateLog* _rateLog) {
  _rateLog->logRates(id_, name(),
                     supplyMonitor_->rate(),
                     injectionMonitor_->rate(),
                     deliveredMonitor_->rate(),
                     ejectionMonitor_->rate());
}

u32 Terminal::messagesSent() const {
  return messagesSent_;
}

u32 Terminal::messagesReceived() const {
  return messagesReceived_;
}

u32 Terminal::transactionsCreated() const {
  return transactionsCreated_;
}

void Terminal::setMessageReceiver(MessageReceiver* _receiver) {
  messageReceiver_ = _receiver;
}

void Terminal::receiveMessage(Message* _message) {
  // log this occurrence
  ejectionMonitor_->monitorMessage(_message);

  // change the owner of the message to this terminal
  _message->setOwner(this);
}

void Terminal::messageEnteredInterface(Message* _message) {
  // log this occurrence
  injectionMonitor_->monitorMessage(_message);
}

void Terminal::messageExitedNetwork(Message* _message) {
  // log this occurrence
  deliveredMonitor_->monitorMessage(_message);

  // remove this message from the outstanding list and count it
  u64 res = outstandingMessages_.erase(_message);
  (void)res;  // unused
  assert(res == 1);
  messagesReceived_++;
}

void Terminal::enrouteCount(u32* _messages, u32* _packets, u32* _flits) const {
  *_messages = 0;
  *_packets = 0;
  *_flits = 0;
  for (auto it = outstandingMessages_.cbegin();
       it != outstandingMessages_.cend(); ++it) {
    *_messages += 1;
    *_packets += (*it)->numPackets();
    *_flits += (*it)->numFlits();
  }
}

u32 Terminal::sendMessage(Message* _message, u32 _destinationId) {
  // log this occurrence
  supplyMonitor_->monitorMessage(_message);

  // set each packet's metadata
  u32 numPackets = _message->numPackets();
  for (u32 pkt = 0; pkt < numPackets; pkt++) {
    Packet* packet = _message->packet(pkt);
    app_->metadataHandler()->packetInjection(app_, packet);
  }

  // handle the message as a whole
  u32 msgId = messagesSent_;
  messagesSent_++;
  _message->setOwner(this);
  _message->setId(msgId);
  _message->setSourceId(id_);
  _message->setSourceAddress(&address_);
  _message->setDestinationId(_destinationId);
  Terminal* dest = application()->getTerminal(_destinationId);
  _message->setDestinationAddress(&dest->address_);
  messageReceiver_->receiveMessage(_message);
  bool res = outstandingMessages_.insert(_message).second;
  (void)res;  // unused
  assert(res);
  return msgId;
}

u64 Terminal::createTransaction() {
  u32 count = transactionsCreated_;
  assert(transactionsCreated_ < U32_MAX);
  transactionsCreated_++;
  return app_->createTransaction(id_, count);
}

void Terminal::endTransaction(u64 _trans) {
  app_->endTransaction(_trans);
}
