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
#include "interface/standard/InputQueue.h"

#include <cassert>

#include "interface/standard/Interface.h"
#include "types/Packet.h"

namespace Standard {

InputQueue::InputQueue(
    const std::string& _name, Interface* _interface,
    CrossbarScheduler* _crossbarScheduler, u32 _crossbarSchedulerIndex,
    Crossbar* _crossbar, u32 _crossbarIndex, u32 _vc)
    : Component(_name, _interface), crossbarScheduler_(_crossbarScheduler),
      crossbarSchedulerIndex_(_crossbarSchedulerIndex), crossbar_(_crossbar),
      crossbarIndex_(_crossbarIndex), interface_(_interface), vc_(_vc) {
}

InputQueue::~InputQueue() {}

void InputQueue::receiveFlit(Flit* _flit) {
  // all flits get staged as packets before continuing in the network
  stageQueue_.push(_flit);

  u64 now = gSim->time();
  bool wasEmpty = sendQueue_.empty();

  // if a full packet has been received, transfer flits to send queue
  if (_flit->isTail()) {
    Flit* f = stageQueue_.front();
    stageQueue_.pop();
    assert(f->isHead());
    f->setSendTime(now);
    sendQueue_.push(f);
    while (!stageQueue_.empty()) {
      f = stageQueue_.front();
      stageQueue_.pop();
      assert(f->isHead() == false);
      f->setSendTime(now);
      sendQueue_.push(f);
    }
  }

  bool isEmpty = sendQueue_.empty();

  // determine if a new event needs to occur
  if (wasEmpty && !isEmpty) {
    assert(gSim->epsilon() == 0);
    addEvent(gSim->time(), 1, nullptr, 0);
  }
}

void InputQueue::crossbarSchedulerResponse(u32 _port, u32 _vc) {
  if (_port != U32_MAX) {
    // granted
    assert(vc_ == _vc);
    Flit* flit = sendQueue_.front();
    sendQueue_.pop();
    crossbar_->inject(flit, crossbarIndex_, 0);
    crossbarScheduler_->decrementCreditCount(_vc);
  } else {
    // denied
  }

  // request if there is more flits in the queue
  if (sendQueue_.size() > 0) {
    addEvent(gSim->time(), 1, nullptr, 0);
  }
}

void InputQueue::processEvent(void* _event, s32 _type) {
  u64 metadata = sendQueue_.front()->getPacket()->getMetadata();
  crossbarScheduler_->request(crossbarSchedulerIndex_, 0, vc_, metadata);
}

}  // namespace Standard
