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
#include "interface/standard/Interface.h"

#include <cassert>

#include "application/Application.h"
#include "interface/standard/InputQueue.h"
#include "interface/standard/MessageReassembler.h"
#include "interface/standard/Ejector.h"
#include "interface/standard/PacketReassembler.h"
#include "network/InjectionAlgorithm.h"
#include "types/MessageOwner.h"

namespace Standard {

Interface::Interface(
    const std::string& _name, const Component* _parent, u32 _id,
    InjectionAlgorithmFactory* _injectionAlgorithmFactory,
    Json::Value _settings)
    : ::Interface(_name, _parent, _id, _settings) {
  injectionAlgorithm_ = _injectionAlgorithmFactory->createInjectionAlgorithm(
      "InjectionAlgorithm", this, this);

  u32 initCredits = _settings["init_credits"].asUInt();
  assert(initCredits > 0);

  assert(_settings.isMember("fixed_msg_vc"));
  fixedMsgVc_ = _settings["fixed_msg_vc"].asBool();

  inputQueues_.resize(numVcs_);
  crossbar_ = new Crossbar("Crossbar", this, numVcs_, 1, _settings["crossbar"]);
  crossbarScheduler_ = new CrossbarScheduler(
      "CrossbarScheduler", this, numVcs_, numVcs_, 1,
      _settings["crossbar_scheduler"]);

  for (u32 vc = 0; vc < numVcs_; vc++) {
    // initialize the credit count in the CrossbarScheduler
    crossbarScheduler_->initCreditCount(vc, initCredits);

    // create the input queue
    inputQueues_.at(vc) = new InputQueue(
        "InputQueue_" + std::to_string(vc), this, crossbarScheduler_, vc,
        crossbar_, vc, vc);

    // link queue to scheduler
    crossbarScheduler_->setClient(vc, inputQueues_.at(vc));
  }

  ejector_ = new Ejector("Ejector", this);
  crossbar_->setReceiver(0, ejector_, 0);

  // create packet reassemblers
  packetReassemblers_.resize(numVcs_);
  for (u32 vc = 0; vc < numVcs_; vc++) {
    std::string tname = "PacketReassembler_" + std::to_string(vc);
    packetReassemblers_.at(vc) = new PacketReassembler(tname, this);
  }

  // create message reassembler
  messageReassembler_ = new MessageReassembler("MessageReassembler", this);
}

Interface::~Interface() {
  delete injectionAlgorithm_;
  delete ejector_;
  delete crossbarScheduler_;
  delete crossbar_;
  for (u32 i = 0; i < numVcs_; i++) {
    delete inputQueues_.at(i);
    delete packetReassemblers_.at(i);
  }
  delete messageReassembler_;
}

void Interface::setInputChannel(Channel* _channel) {
  inputChannel_ = _channel;
  _channel->setSink(this, 0);
}

void Interface::setOutputChannel(Channel* _channel) {
  outputChannel_ = _channel;
  _channel->setSource(this, 0);
}

void Interface::receiveMessage(Message* _message) {
  dbgprintf("received message from terminal");
  assert(_message != nullptr);
  u64 now = gSim->time();

  // mark all flit send times
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->getPacket(p);
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      flit->setSendTime(now);
    }
  }

  // issue an injection algorithm request
  InjectionAlgorithm::Response* resp = new InjectionAlgorithm::Response();
  injectionAlgorithm_->request(this, _message, resp);
}

void Interface::injectionAlgorithmResponse(
    Message* _message, InjectionAlgorithm::Response* _response) {
  // push all flits into the corresponding input queue
  u32 pktVc = U32_MAX;
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->getPacket(p);

    // get the packet's VC
    if (!fixedMsgVc_ || pktVc == U32_MAX) {
      _response->get(gSim->rnd.nextU64(0, _response->size()-1), &pktVc);
    }

    // apply VC and inject
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      flit->setVc(pktVc);
      inputQueues_.at(pktVc)->receiveFlit(flit);
    }
  }

  delete _response;
}

void Interface::sendFlit(u32 _port, Flit* _flit) {
  assert(_port == 0);
  assert(outputChannel_->getNextFlit() == nullptr);
  outputChannel_->setNextFlit(_flit);
}

void Interface::receiveFlit(u32 _port, Flit* _flit) {
  assert(_port == 0);
  assert(_flit != nullptr);

  // send a credit back
  sendCredit(_port, _flit->getVc());

  // check destination is correct
  u32 dest = _flit->getPacket()->getMessage()->getDestinationId();
  assert(dest == getId());

  // mark the receive time
  _flit->setReceiveTime(gSim->time());

  // process flit, attempt to create packet
  Packet* packet = packetReassemblers_.at(_flit->getVc())->receiveFlit(_flit);
  // if a packet was completed, process it
  if (packet) {
    // process packet, attempt to create message
    Message* message = messageReassembler_->receivePacket(packet);
    if (message) {
      getMessageReceiver()->receiveMessage(message);
    }
  }
}

void Interface::sendCredit(u32 _port, u32 _vc) {
  assert(_port == 0);
  assert(_vc < numVcs_);

  // send credit
  Credit* credit = inputChannel_->getNextCredit();
  if (credit == nullptr) {
    credit = new Credit(numVcs_);
    inputChannel_->setNextCredit(credit);
  }
  credit->putNum(_vc);
}

void Interface::receiveCredit(u32 _port, Credit* _credit) {
  assert(_port == 0);
  while (_credit->more()) {
    u32 vc = _credit->getNum();
    dbgprintf("port = %u, vc = %u", _port, vc);
    crossbarScheduler_->incrementCreditCount(vc);
  }
  delete _credit;
}

}  // namespace Standard
