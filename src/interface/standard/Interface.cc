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
#include "interface/standard/Interface.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "architecture/util.h"
#include "workload/Application.h"
#include "interface/standard/Ejector.h"
#include "interface/standard/MessageReassembler.h"
#include "interface/standard/OutputQueue.h"
#include "interface/standard/PacketReassembler.h"
#include "types/MessageOwner.h"

// event types
#define INJECT_MESSAGE (0x45)

namespace Standard {

Interface::Interface(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, u32 _numVcs,
    const std::vector<std::tuple<u32, u32> >& _protocolClassVcs,
    MetadataHandler* _metadataHandler, Json::Value _settings)
    : ::Interface(_name, _parent, _id, _address, _numVcs, _protocolClassVcs,
                  _metadataHandler, _settings) {
  // init credits
  initCredits_ = 0;
  inputQueueTailored_ = false;
  inputQueueMult_ = 0;
  inputQueueMax_ = 0;
  inputQueueMin_ = 0;
  assert(_settings.isMember("init_credits_mode"));

  if (_settings["init_credits_mode"].asString() == "tailored") {
    inputQueueTailored_ = true;
    inputQueueMult_ = _settings["init_credits"].asDouble();
    assert(inputQueueMult_ > 0.0);
    // max and min queue depth
    assert(_settings.isMember("credits_min"));
    inputQueueMin_ = _settings["credits_min"].asUInt();
    assert(_settings.isMember("credits_max"));
    inputQueueMax_ = _settings["credits_max"].asUInt();
    assert(inputQueueMin_ <= inputQueueMax_);
  } else if (_settings["init_credits_mode"].asString() == "fixed") {
    inputQueueTailored_ = false;
    initCredits_ = _settings["init_credits"].asUInt();
    assert(initCredits_ > 0);
  } else {
    fprintf(stderr, "Wrong init credits mode, options: tailor or fixed\n");
    assert(false);
  }

  assert(_settings.isMember("adaptive"));
  adaptive_ = _settings["adaptive"].asBool();

  assert(_settings.isMember("fixed_msg_vc"));
  fixedMsgVc_ = _settings["fixed_msg_vc"].asBool();

  // create the crossbar and scheduler
  crossbar_ = new Crossbar("Crossbar", this, numVcs_, 1,
                           Simulator::Clock::CHANNEL, _settings["crossbar"]);
  crossbarScheduler_ = new CrossbarScheduler(
      "CrossbarScheduler", this, numVcs_, numVcs_, 1, 0,
      Simulator::Clock::CHANNEL, _settings["crossbar_scheduler"]);

  // create the output queues
  outputQueues_.resize(numVcs_, nullptr);
  queueOccupancy_.resize(numVcs_, 0);
  for (u32 vc = 0; vc < numVcs_; vc++) {
    // create the output queue
    outputQueues_.at(vc) = new OutputQueue(
        "OutputQueue_" + std::to_string(vc), this, crossbarScheduler_, vc,
        crossbar_, vc, vc);

    // link queue to scheduler
    crossbarScheduler_->setClient(vc, outputQueues_.at(vc));
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
  delete ejector_;
  delete crossbarScheduler_;
  delete crossbar_;
  for (u32 i = 0; i < numVcs_; i++) {
    delete outputQueues_.at(i);
    delete packetReassemblers_.at(i);
  }
  delete messageReassembler_;
}

void Interface::setInputChannel(u32 _port, Channel* _channel) {
  assert(_port == 0);
  inputChannel_ = _channel;
  _channel->setSink(this, 0);
}

Channel* Interface::getInputChannel(u32 _port) const {
  assert(_port == 0);
  return inputChannel_;
}

void Interface::setOutputChannel(u32 _port, Channel* _channel) {
  assert(_port == 0);
  outputChannel_ = _channel;
  _channel->setSource(this, 0);
}

Channel* Interface::getOutputChannel(u32 _port) const {
  assert(_port == 0);
  return outputChannel_;
}

void Interface::receiveMessage(Message* _message) {
  assert(_message != nullptr);
  assert(gSim->epsilon() == 0);

  u64 now = gSim->time();

  // mark all flit send times
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);
    packetArrival(packet);  // inform the base class of arrival
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      flit->setSendTime(now);
    }
  }

  // retrieve the protocol class of the message
  u32 protocolClass = _message->getProtocolClass();
  assert(protocolClass < protocolClassVcs_.size());

  // use the protocol class to set the injection VC(s)
  u32 pktVc = U32_MAX;
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);

    // get the packet's VC
    if (!fixedMsgVc_ || pktVc == U32_MAX) {
      // retrieve the VC range based on protocol class specification
      u32 baseVc = std::get<0>(protocolClassVcs_.at(protocolClass));
      u32 numVcs = std::get<1>(protocolClassVcs_.at(protocolClass));

      // choose VC
      if (adaptive_) {
        // find all minimally congested VCs within the protocol class
        std::vector<u32> minOccupancyVcs;
        u32 minOccupancy = U32_MAX;
        for (u32 vc = baseVc; vc < baseVc + numVcs; vc++) {
          u32 occupancy = queueOccupancy_.at(vc);
          if (occupancy < minOccupancy) {
            minOccupancy = occupancy;
            minOccupancyVcs.clear();
          }
          if (occupancy <= minOccupancy) {
            minOccupancyVcs.push_back(vc);
          }
        }

        // choose randomly among the minimally congested VCs
        assert(minOccupancyVcs.size() > 0);
        u32 rnd = gSim->rnd.nextU64(0, minOccupancyVcs.size() - 1);
        pktVc = minOccupancyVcs.at(rnd);
      } else {
        // choose a random VC within the protocol class
        pktVc = gSim->rnd.nextU64(baseVc, baseVc + numVcs - 1);
      }
    }

    // apply VC and inject
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      flit->setVc(pktVc);
    }

    // update credit counts
    assert(queueOccupancy_.at(pktVc) < U32_MAX - packet->numFlits());
    queueOccupancy_.at(pktVc) += packet->numFlits();
  }

  // create an event to inject the message into the queues
  assert(gSim->epsilon() == 0);
  addEvent(gSim->time(), 1, _message, INJECT_MESSAGE);
}

void Interface::initialize() {
  // init credits
  assert(outputChannel_->latency());
  // compute tailored queue depth for donwstream channel
  u32 credits = initCredits_;
  u32 channelLatency = outputChannel_->latency();
  if (inputQueueTailored_) {
    credits = computeTailoredBufferLength(
            inputQueueMult_, inputQueueMin_, inputQueueMax_, channelLatency);
  }
  dbgprintf("INTF out_chan: %u x %.2f = credits %u",
            channelLatency, inputQueueMult_, credits);
  for (u32 vc = 0; vc < numVcs_; vc++) {
    // initialize the credit count in the CrossbarScheduler
    crossbarScheduler_->initCredits(vc, credits);
  }
}

void Interface::sendFlit(u32 _port, Flit* _flit) {
  assert(_port == 0);
  assert(outputChannel_->getNextFlit() == nullptr);
  outputChannel_->setNextFlit(_flit);

  // inform the base class of departure
  if (_flit->isHead()) {
    packetDeparture(_flit->packet());
  }

  // check source is correct
  u32 src = _flit->packet()->message()->getSourceId();
  (void)src;  // unused
  assert(src == id_);
}

void Interface::receiveFlit(u32 _port, Flit* _flit) {
  assert(_port == 0);
  assert(_flit != nullptr);

  // send a credit back
  sendCredit(_port, _flit->getVc());

  // check destination is correct
  u32 dest = _flit->packet()->message()->getDestinationId();
  (void)dest;  // unused
  assert(dest == id_);

  // mark the receive time
  _flit->setReceiveTime(gSim->time());

  // process flit, attempt to create packet
  Packet* packet = packetReassemblers_.at(_flit->getVc())->receiveFlit(_flit);
  // if a packet was completed, process it
  if (packet) {
    // process packet, attempt to create message
    Message* message = messageReassembler_->receivePacket(packet);
    if (message) {
      messageReceiver()->receiveMessage(message);
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
    // dbgprintf("port = %u, vc = %u", _port, vc);
    crossbarScheduler_->incrementCredit(vc);
  }
  delete _credit;
}

void Interface::incrementCredit(u32 _vc) {
  assert(_vc < numVcs_);
  assert(queueOccupancy_.at(_vc) > 0);
  queueOccupancy_.at(_vc)--;
}

void Interface::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case INJECT_MESSAGE:
      assert(gSim->epsilon() == 1);
      injectMessage(reinterpret_cast<Message*>(_event));
      break;

    default:
      assert(false);
  }
}

void Interface::injectMessage(Message* _message) {
  for (u32 p = 0; p < _message->numPackets(); p++) {
    Packet* packet = _message->packet(p);
    for (u32 f = 0; f < packet->numFlits(); f++) {
      Flit* flit = packet->getFlit(f);
      outputQueues_.at(flit->getVc())->receiveFlit(0, flit);
    }
  }
}

}  // namespace Standard

registerWithObjectFactory("standard", ::Interface,
                          Standard::Interface, INTERFACE_ARGS);
