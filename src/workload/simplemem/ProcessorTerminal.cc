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
#include "workload/simplemem/ProcessorTerminal.h"

#include <cassert>

#include "workload/simplemem/Application.h"
#include "workload/simplemem/MemoryOp.h"
#include "event/Simulator.h"
#include "types/Message.h"
#include "types/Packet.h"
#include "types/Flit.h"

namespace SimpleMem {

ProcessorTerminal::ProcessorTerminal(
    const std::string& _name, const Component* _parent, u32 _id,
    const std::vector<u32>& _address, ::Application* _app,
    Json::Value _settings)
    : ::Terminal(_name, _parent, _id, _address, _app) {
  // protocol class of injection
  assert(_settings.isMember("protocol_class"));
  protocolClass_ = _settings["protocol_class"].asUInt();

  // latency and memory access
  latency_ = _settings["latency"].asUInt();
  assert(latency_ > 0);
  numMemoryAccesses_ = _settings["memory_accesses"].asUInt();
  remainingAccesses_ = numMemoryAccesses_;

  // initialize state machine
  fsm_ = eState::kDone;
}

ProcessorTerminal::~ProcessorTerminal() {
}

void ProcessorTerminal::processEvent(void* _event, s32 _type) {
  startNextMemoryAccess();
}

f64 ProcessorTerminal::percentComplete() const {
  f64 pc = 1.0 - ((f64)remainingAccesses_ / (f64)numMemoryAccesses_);
  dbgprintf("Percent complete: %f", pc);
  return pc;
}

void ProcessorTerminal::start() {
  continueProcessing();
}

void ProcessorTerminal::continueProcessing() {
  if (remainingAccesses_ > 0) {
    dbgprintf("starting processing");
    addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, latency_),
             0, nullptr, 0);
    fsm_ = eState::kProcessing;
  } else {
    Application* app = reinterpret_cast<Application*>(application());
    app->processorComplete(id_);
  }
}

void ProcessorTerminal::handleDeliveredMessage(Message* _message) {
  // I don't need to care about this
}

void ProcessorTerminal::handleReceivedMessage(Message* _message) {
  dbgprintf("received message");

  // log the message
  Application* app = reinterpret_cast<Application*>(application());
  app->workload()->messageLog()->logMessage(_message);

  // verify the message
  MemoryOp* memOp = reinterpret_cast<MemoryOp*>(_message->getData());
  assert(memOp != nullptr);
  if (fsm_ == eState::kWaitingForReadResp) {
    assert(memOp->op() == MemoryOp::eOp::kReadResp);
  } else if (fsm_ == eState::kWaitingForWriteResp) {
    assert(memOp->op() == MemoryOp::eOp::kWriteResp);
  } else {
    assert(false);
  }

  // end the transaction
  endTransaction(_message->getTransaction());
  app->workload()->messageLog()->endTransaction(_message->getTransaction());

  delete memOp;
  delete _message;

  remainingAccesses_--;
  dbgprintf("remaining accesses = %u", remainingAccesses_);
  continueProcessing();
}

void ProcessorTerminal::startNextMemoryAccess() {
  assert(remainingAccesses_ > 0);

  Application* app = reinterpret_cast<Application*>(application());
  u32 totalMemory = app->totalMemory();
  u32 memorySlice = app->memorySlice();
  u32 blockSize = app->blockSize();
  u32 bytesPerFlit = app->bytesPerFlit();
  u32 headerOverhead = app->headerOverhead();
  u32 maxPacketSize = app->maxPacketSize();

  // generate a memory request
  u32 address = gSim->rnd.nextU64(0, totalMemory - 1);
  address &= ~(blockSize - 1);  // align to blockSize
  MemoryOp::eOp op = gSim->rnd.nextBool() ?
                     MemoryOp::eOp::kReadReq : MemoryOp::eOp::kWriteReq;
  MemoryOp* memOp = new MemoryOp(op, address, blockSize);
  if (op == MemoryOp::eOp::kWriteReq) {
    u8* block = memOp->block();
    for (u64 i = 0; i < blockSize; i++) {
      block[i] = (u8)gSim->rnd.nextU64(0, 255);
    }
    fsm_ = eState::kWaitingForWriteResp;
  } else {
    fsm_ = eState::kWaitingForReadResp;
  }

  // determine the proper memory terminal
  u64 memoryTerminalId = address / memorySlice;
  memoryTerminalId *= 2;

  // determine message length
  u32 messageLength = headerOverhead + 1 + sizeof(u32) + blockSize;
  messageLength /= bytesPerFlit;
  u32 numPackets = messageLength / maxPacketSize;
  if ((messageLength % maxPacketSize) > 0) {
    numPackets++;
  }

  // create network message, packets, and flits
  Message* message = new Message(numPackets, memOp);
  message->setProtocolClass(protocolClass_);
  u64 trans = createTransaction();
  message->setTransaction(trans);
  app->workload()->messageLog()->startTransaction(trans);

  u32 flitsLeft = messageLength;
  for (u32 p = 0; p < numPackets; p++) {
    u32 packetLength = flitsLeft > maxPacketSize ? maxPacketSize : flitsLeft;
    Packet* packet = new Packet(p, packetLength, message);
    message->setPacket(p, packet);

    for (u32 f = 0; f < packetLength; f++) {
      bool headFlit = f == 0;
      bool tailFlit = f == (packetLength - 1);
      Flit* flit = new Flit(f, headFlit, tailFlit, packet);
      packet->setFlit(f, flit);
    }
    flitsLeft -= packetLength;
  }

  // send the request to the memory terminal
  dbgprintf("sending %s request to %u (address %u)",
            (op == MemoryOp::eOp::kWriteReq) ?
            "write" : "read", memoryTerminalId, address);
  sendMessage(message, memoryTerminalId);
}

}  // namespace SimpleMem
