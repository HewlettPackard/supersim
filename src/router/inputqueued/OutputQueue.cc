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
#include "router/inputqueued/OutputQueue.h"

#include <cassert>

#include <string>
#include <queue>
#include <algorithm>

#include "router/inputqueued/Router.h"
#include "types/Packet.h"

// event types
#define PROCESS_PIPELINE (0xB7)

namespace InputQueued {

OutputQueue::OutputQueue(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _depth, u32 _port)
    : Component(_name, _parent), depth_(_depth), port_(_port), router_(_router),
      lastReceivedTime_(U64_MAX) {
  // ensure the buffer is empty
  assert(buffer_.size() == 0);

  // no event is set to trigger
  eventTime_ = U64_MAX;
}

OutputQueue::~OutputQueue() {}

void OutputQueue::receiveFlit(u32 _port, Flit* _flit) {
  assert(gSim->epsilon() == 1);

  // 'port' is unused
  assert(_port == 0);

  // we can only receive one flit per cycle
  assert((lastReceivedTime_ == U64_MAX) ||
         (lastReceivedTime_ < gSim->time()));
  lastReceivedTime_ = gSim->time();

  // push flit into corresponding buffer
  buffer_.push(_flit);
  assert(buffer_.size() <= depth_);  // overflow check

  // ensure an event is set to process the pipeline
  if (eventTime_ == U64_MAX) {
    eventTime_ = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
    addEvent(eventTime_, 2, nullptr, PROCESS_PIPELINE);
  }
}

void OutputQueue::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case (PROCESS_PIPELINE):
      assert(gSim->epsilon() == 2);
      processPipeline();
      break;

    default:
      assert(false);
  }
}

void OutputQueue::processPipeline() {
  /*
   * Send the next flit on the output channel
   */
  Flit* flit = buffer_.front();
  buffer_.pop();
  router_->sendFlit(port_, flit);

  // clear the eventTime_ variable to indicate no more events are set
  eventTime_ = U64_MAX;

  /*
   * there is one reason that the next cycle should be processed:
   *  1. more flits in the queue, need to pull one out
   * if this is true, create and expect an event the next cycle
   */
  if (buffer_.size() > 0) {  // more flits in buffer
    // set a pipeline event for the next cycle
    eventTime_ = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
    addEvent(eventTime_, 2, nullptr, PROCESS_PIPELINE);
  }
}

}  // namespace InputQueued
