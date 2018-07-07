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
#include "workload/stencil/StencilTerminal.h"

#include <bits/bits.h>
#include <mut/mut.h>

#include <cassert>
#include <cmath>

#include <algorithm>

#include "network/Network.h"
#include "stats/MessageLog.h"
#include "types/Flit.h"
#include "types/Packet.h"
#include "workload/stencil/Application.h"
#include "workload/util.h"

#define kFinishComputeEvt  (0xF8)
#define kTransitionFsmEvt  (0xA1)
#define kReceiveMessageEvt (0x3C)

namespace Stencil {

StencilTerminal::StencilTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, const std::vector<u32>* _termToProc,
    const std::vector<u32>* _procToTerm,
    const std::vector<std::tuple<u32, u32> >& _exchangeSendMessages,
    u32 _exchangeRecvMessages, ::Application* _app, Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app),
      numIterations_(_settings["num_iterations"].asUInt()),
      maxPacketSize_(_settings["max_packet_size"].asUInt()),
      termToProc_(_termToProc), procToTerm_(_procToTerm),
      exchangeSendMessages_(_exchangeSendMessages),
      exchangeRecvMessages_(_exchangeRecvMessages),
      collectiveSize_(_settings["collective_size"].asUInt()),
      exchangeProtocolClass_(_settings["exchange_protocol_class"].asUInt()),
      collectiveProtocolClass_(_settings["collective_protocol_class"].asUInt()),
      computeDelay_(_settings["compute_delay"].asUInt64()) {
  // check settings
  assert(numIterations_ > 0);
  assert(maxPacketSize_ > 0);
  assert(!_settings["collective_size"].isNull());
  assert(!_settings["exchange_protocol_class"].isNull());
  assert(!_settings["collective_protocol_class"].isNull());
  assert(computeDelay_ > 0);  // this avoids massive recursion

  // initialize state
  iteration_ = 0;
  fsm_ = StencilTerminal::Fsm::kWaiting;
  collectiveOffset_ = U32_MAX;

  dbgprintf("");
  dbgprintf("Terminal=%u Process=%u", id(), termToProc_->at(id()));
  dbgprintf("Exchange Send Messages:");
  for (const std::tuple<u32, u32>& sm : exchangeSendMessages_) {
    dbgprintf("  Dst=%u Size=%u", std::get<0>(sm), std::get<1>(sm));
  }
  dbgprintf("Exchange Recv Messages: %u", exchangeRecvMessages_);
}

StencilTerminal::~StencilTerminal() {
  assert(fsm_ == StencilTerminal::Fsm::kDone);
}

void StencilTerminal::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case kFinishComputeEvt:
      assert(_event == nullptr);
      finishCompute();
      break;
    case kTransitionFsmEvt:
      assert(_event == nullptr);
      transitionFsm();
      break;
    case kReceiveMessageEvt:
      assert(_event != nullptr);
      processReceivedMessage(reinterpret_cast<Message*>(_event));
      break;
    default:
      assert(false);
  }
}

f64 StencilTerminal::percentComplete() const {
  return (f64)iteration_ / (f64)numIterations_;
}

void StencilTerminal::start() {
  assert(fsm_ == StencilTerminal::Fsm::kWaiting);
  advanceFsm();
}

void StencilTerminal::handleDeliveredMessage(Message* _message) {
  // log the message/transaction
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->logMessage(_message);
  app->workload()->messageLog()->endTransaction(_message->getTransaction());
}

void StencilTerminal::handleReceivedMessage(Message* _message) {
  // this is epsilon 1, must handle on 0
  assert(gSim->epsilon() == 1);
  addEvent(gSim->time() + 1, 0, _message, kReceiveMessageEvt);
}

void StencilTerminal::advanceFsm() {
  addEvent(gSim->time() + 1, 0, nullptr, kTransitionFsmEvt);
}

void StencilTerminal::transitionFsm() {
  StencilTerminal::Fsm nextState;
  switch (fsm_) {
    case StencilTerminal::Fsm::kWaiting:
      nextState = StencilTerminal::Fsm::kCompute;
      break;
    case StencilTerminal::Fsm::kCompute:
      if (exchangeSendMessages_.size() > 0 || exchangeRecvMessages_ > 0) {
        nextState = StencilTerminal::Fsm::kExchange;
      } else if (collectiveSize_ > 0) {
        nextState = StencilTerminal::Fsm::kCollective;
      } else if (++iteration_ == numIterations_) {
        nextState = StencilTerminal::Fsm::kDone;
      } else {
        nextState = StencilTerminal::Fsm::kCompute;
      }
      break;
    case StencilTerminal::Fsm::kExchange:
      if (collectiveSize_ > 0) {
        nextState = StencilTerminal::Fsm::kCollective;
      } else if (++iteration_ == numIterations_) {
        nextState = StencilTerminal::Fsm::kDone;
      } else {
        nextState = StencilTerminal::Fsm::kCompute;
      }
      break;
    case StencilTerminal::Fsm::kCollective:
      if (++iteration_ == numIterations_) {
        nextState = StencilTerminal::Fsm::kDone;
      } else {
        nextState = StencilTerminal::Fsm::kCompute;
      }
      break;
    case StencilTerminal::Fsm::kDone:
      assert(false);  // programmer error!
    default:
      assert(false);  // programmer error!
  }

  fsm_ = nextState;

  switch (fsm_) {
    case StencilTerminal::Fsm::kWaiting:
      assert(false);  // programmer error!
      break;
    case StencilTerminal::Fsm::kCompute:
      dbgprintf("starting compute");
      startCompute();
      break;
    case StencilTerminal::Fsm::kExchange:
      dbgprintf("starting exchange");
      startExchange();
      break;
    case StencilTerminal::Fsm::kCollective:
      dbgprintf("starting collective");
      startCollective();
      break;
    case StencilTerminal::Fsm::kDone:
      dbgprintf("done");
      break;
    default:
      assert(false);  // programmer error!
  }
}

void StencilTerminal::processReceivedMessage(Message* _message) {
  switch (decodeState(_message->getOpCode())) {
    case StencilTerminal::Fsm::kExchange:
      handleExchangeMessage(_message);
      break;
    case StencilTerminal::Fsm::kCollective:
      handleCollectiveMessage(_message);
      break;
    default:
      assert(false);  // programmer error!
  }
}

void StencilTerminal::startCompute() {
  // determine computation time
  // TODO(nic): make this a probability distribution
  u64 time = gSim->time() + computeDelay_;
  addEvent(time, 0, nullptr, kFinishComputeEvt);
}

void StencilTerminal::finishCompute() {
  // advance the state
  advanceFsm();
}

void StencilTerminal::startExchange() {
  // enqueue all messages to be sent
  for (const auto& t : exchangeSendMessages_) {
    u32 destProc = std::get<0>(t);
    u32 size = std::get<1>(t);
    generateMessage(procToTerm_->at(destProc), size, exchangeProtocolClass_,
                    encodeOp(fsm_, iteration_));
  }

  // check if all messages have been received already
  if (exchangeRecvCount_[iteration_] == exchangeRecvMessages_) {
    finishExchange();
  } else {
    // do nothing, the reception of messages will trigger advancement
  }
}

void StencilTerminal::handleExchangeMessage(Message* _message) {
  u32 msgIter = decodeIteration(_message->getOpCode());

  // increment the received message count
  exchangeRecvCount_[msgIter]++;
  dbgprintf("received for iter=%u, count=%u", msgIter,
            exchangeRecvCount_[iteration_]);

  // delete the message
  delete _message;

  // determine if exchange is complete (must be in exchange state)
  if ((fsm_ == StencilTerminal::Fsm::kExchange) &&
      (exchangeRecvCount_[iteration_] == exchangeRecvMessages_)) {
    finishExchange();
  }
}

void StencilTerminal::finishExchange() {
  // remove the receive state for this iteration
  u32 res = exchangeRecvCount_.erase(iteration_);
  (void)res;  // unused
  assert(res == 1 || exchangeRecvMessages_ == 0);

  // advance the state
  advanceFsm();
}

void StencilTerminal::startCollective() {
  // send the first offset
  collectiveOffset_ = 1;
  u32 destProc = collectiveDestination(collectiveOffset_);
  generateMessage(procToTerm_->at(destProc),
                  collectiveSize_, collectiveProtocolClass_,
                  encodeOp(fsm_, iteration_));

  // if something has already been received, trigger handling
  handleCollectiveMessage(nullptr);
}

void StencilTerminal::handleCollectiveMessage(Message* _message) {
  if (_message) {
    dbgprintf("received collective message");

    // stash the message in its iteration
    u32 msgIter = decodeIteration(_message->getOpCode());
    u32 msgSrc = termToProc_->at(_message->getSourceId());
    bool res = collectiveRecv_[msgIter].insert(msgSrc).second;
    (void)res;  // unused
    assert(res);

    // delete the message
    delete _message;
  }

  // check if we've received from the right source
  if (fsm_ == StencilTerminal::Fsm::kCollective) {
    // process as much as possible
    while (true) {
      u32 source = collectiveSource(collectiveOffset_);
      if (collectiveRecv_[iteration_].count(source) > 0) {
        // advance phase
        collectiveOffset_ <<= 1;

        // check for collective completion
        if (collectiveOffset_ >= application()->numTerminals()) {
          finishCollective();
          break;
        }

        // send next message
        u32 dstProc = collectiveDestination(collectiveOffset_);
        generateMessage(procToTerm_->at(dstProc),
                        collectiveSize_, collectiveProtocolClass_,
                        encodeOp(fsm_, iteration_));
      } else {
        // not yet received, break now and wait
        break;
      }
    }
  }
}

void StencilTerminal::finishCollective() {
  // remove the receive state for this iteration
  assert(collectiveRecv_.at(iteration_).size() ==
         bits::ceilLog2(application()->numTerminals()));
  u32 res = collectiveRecv_.erase(iteration_);
  (void)res;  // unused
  assert(res == 1);

  // advance the state
  advanceFsm();
}

void StencilTerminal::generateMessage(u32 _destTerminal, u32 _size,
                                      u32 _protocolClass, u32 _msgType) {
  u32 destination = _destTerminal;
  u32 messageSize = _size;
  u32 protocolClass = _protocolClass;
  u64 transaction = createTransaction();
  u32 msgType = _msgType;

  // start the transaction in the application
  application()->workload()->messageLog()->startTransaction(transaction);

  // determine the number of packets
  u32 numPackets = messageSize / maxPacketSize_;
  if ((messageSize % maxPacketSize_) > 0) {
    numPackets++;
  }

  // create the message object
  Message* message = new Message(numPackets, nullptr);
  message->setProtocolClass(protocolClass);
  message->setTransaction(transaction);
  message->setOpCode(msgType);

  // create the packets
  u32 flitsLeft = messageSize;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize_ ?
                       maxPacketSize_ : flitsLeft;

    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    // create flits
    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the message
  u32 msgId = sendMessage(message, destination);
  (void)msgId;  // unused
}

u32 StencilTerminal::encodeOp(StencilTerminal::Fsm _state, u32 _iteration) {
  assert(_iteration < ((u32)1 << 30));

  switch (_state) {
    case StencilTerminal::Fsm::kExchange:
      return _iteration;
    case StencilTerminal::Fsm::kCollective:
      return ((u32)1 << 31) | _iteration;
    default:
      assert(false);  // programmer error
  }
}

StencilTerminal::Fsm StencilTerminal::decodeState(u32 _opCode) {
  switch (_opCode >> 31) {
    case 0:
      return StencilTerminal::Fsm::kExchange;
    case 1:
      return StencilTerminal::Fsm::kCollective;
    default:
      assert(false);  // programmer error
  }
}

u32 StencilTerminal::decodeIteration(u32 _opCode) {
  return _opCode & 0x7FFFFFFF;
}

u32 StencilTerminal::collectiveDestination(u32 _offset) {
  u32 myProc = termToProc_->at(id_);
  dbgprintf("sending collective to %u",
            (myProc + _offset) % application()->numTerminals());
  return (myProc + _offset) % application()->numTerminals();
}

u32 StencilTerminal::collectiveSource(u32 _offset) {
  u32 myProc = termToProc_->at(id_);
  if (_offset > myProc) {
    return (myProc + application()->numTerminals()) - _offset;
  } else {
    return (myProc - _offset);
  }
}

}  // namespace Stencil
