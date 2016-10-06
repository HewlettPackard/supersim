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
#include "router/inputoutputqueued/OutputQueue.h"

#include <cassert>

#include <string>
#include <queue>
#include <algorithm>

#include "router/inputoutputqueued/Router.h"
#include "types/Packet.h"

// event types
#define INJECTED_FLIT (0x33)
#define PROCESS_PIPELINE (0xB7)

namespace InputOutputQueued {

OutputQueue::OutputQueue(
    const std::string& _name, const Component* _parent, u32 _depth, u32 _port,
    u32 _vc, CrossbarScheduler* _outputCrossbarScheduler,
    u32 _crossbarSchedulerIndex, Crossbar* _crossbar, u32 _crossbarIndex,
    CrossbarScheduler* _mainCrossbarScheduler, u32 _mainCrossbarVcId)
    : Component(_name, _parent), depth_(_depth), port_(_port), vc_(_vc),
      outputCrossbarScheduler_(_outputCrossbarScheduler),
      crossbarSchedulerIndex_(_crossbarSchedulerIndex), crossbar_(_crossbar),
      crossbarIndex_(_crossbarIndex),
      mainCrossbarScheduler_(_mainCrossbarScheduler),
      mainCrossbarVcId_(_mainCrossbarVcId),
      lastReceivedTime_(U64_MAX) {
  // ensure the buffer is empty
  assert(buffer_.size() == 0);

  // initialize the entry
  swa_.fsm = ePipelineFsm::kEmpty;
  swa_.flit = nullptr;

  // no event is set to trigger
  eventTime_ = U64_MAX;
}

OutputQueue::~OutputQueue() {}

void OutputQueue::receiveFlit(u32 _port, Flit* _flit) {
  assert(gSim->epsilon() == 1);

  // 'port' is unused
  assert(_port == 0);

  // make sure this is the right VC
  assert(_flit->getVc() == vc_);

  // we can only receive one flit per cycle
  assert((lastReceivedTime_ == U64_MAX) ||
         (lastReceivedTime_ < gSim->time()));
  lastReceivedTime_ = gSim->time();

  // push flit into corresponding buffer
  buffer_.push(_flit);
  assert(buffer_.size() <= depth_);  // overflow check

  // queue an event to be notified about the injected flit
  //  this synchronized the two clock domains
  addEvent(gSim->futureCycle(Simulator::Clock::CHANNEL, 1),
           1, _flit, INJECTED_FLIT);
}

void OutputQueue::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case (INJECTED_FLIT):
      assert(gSim->epsilon() == 1);
      setPipelineEvent();
      break;

    case (PROCESS_PIPELINE):
      assert(gSim->epsilon() == 2);
      processPipeline();
      break;

    default:
      assert(false);
  }
}

void OutputQueue::crossbarSchedulerResponse(u32 _port, u32 _vc) {
  assert(swa_.fsm == ePipelineFsm::kWaitingForResponse);

  if (_port != U32_MAX) {
    // granted
    assert(_port == 0);  // only one port here
    assert(_vc == vc_);  // same VC as this
    swa_.fsm = ePipelineFsm::kReadyToAdvance;
  } else {
    // denied
    swa_.fsm = ePipelineFsm::kWaitingToRequest;
  }

  // ensure an event is set to process the pipeline
  setPipelineEvent();
}

void OutputQueue::setPipelineEvent() {
  if (eventTime_ == U64_MAX) {
    eventTime_ = gSim->time();
    addEvent(eventTime_, 2, nullptr, PROCESS_PIPELINE);
  }
}

void OutputQueue::processPipeline() {
  // make sure the pipeline is being processed on clock cycle boundaries
  assert(gSim->time() % gSim->cycleTime(Simulator::Clock::CHANNEL) == 0);

  /*
   * attempt to load the crossbar
   */
  if (swa_.fsm == ePipelineFsm::kReadyToAdvance) {
    // dbgprintf("loading crossbar");

    // send the flit on the crossbar
    crossbar_->inject(swa_.flit, crossbarIndex_, 0);
    outputCrossbarScheduler_->decrementCreditCount(vc_);

    // clear SWA info
    swa_.fsm = ePipelineFsm::kEmpty;
    swa_.flit = nullptr;
  }

  /*
   * attempt to load SWA stage
   */
  if ((swa_.fsm == ePipelineFsm::kEmpty) && (buffer_.empty() == false)) {
    // dbgprintf("loading SWA");

    // ensure SWA is empty
    assert(swa_.flit == nullptr);

    // pull out the front flit
    Flit* flit = buffer_.front();
    buffer_.pop();
    mainCrossbarScheduler_->incrementCreditCount(mainCrossbarVcId_);

    // put it in this pipeline stage
    swa_.flit = flit;

    // set SWA info
    swa_.fsm = ePipelineFsm::kWaitingToRequest;
  }

  /*
   * Attempt to submit a SWA request
   */
  if (swa_.fsm == ePipelineFsm::kWaitingToRequest) {
    swa_.fsm = ePipelineFsm::kWaitingForResponse;
    outputCrossbarScheduler_->request(crossbarSchedulerIndex_, 0, vc_,
                                      swa_.flit);
  }

  // clear the eventTime_ variable to indicate no more events are set
  eventTime_ = U64_MAX;

  /*
   * there are a few reasons that the next cycle should be processed:
   *  1. no credits were available for crossing the switch, try again
   *  2. more flits in the queue, need to pull one out
   * if any of these cases are true, create and expect an event the next cycle
   */
  if ((swa_.fsm == ePipelineFsm::kWaitingToRequest) ||  // no credits
      (buffer_.size() > 0)) {   // more flits in buffer
    // set a pipeline event for the next cycle
    eventTime_ = gSim->futureCycle(Simulator::Clock::CHANNEL, 1);
    addEvent(eventTime_, 2, nullptr, PROCESS_PIPELINE);
  }
}

}  // namespace InputOutputQueued
