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
#include "router/outputqueued/InputQueue.h"

#include <cassert>

#include <algorithm>

#include "network/Network.h"
#include "router/outputqueued/Router.h"
#include "types/Packet.h"

// event types
#define INJECTED_FLIT (0x33)
#define PROCESS_PIPELINE (0xB7)

namespace OutputQueued {

InputQueue::InputQueue(
    const std::string& _name, const Component* _parent, Router* _router,
    u32 _depth, u32 _port, u32 _numVcs, u32 _vc,
    RoutingAlgorithm* _routingAlgorithm)
    : Component(_name, _parent), depth_(_depth), port_(_port), numVcs_(_numVcs),
      vc_(_vc), router_(_router), routingAlgorithm_(_routingAlgorithm) {
  // ensure the buffer is empty
  assert(buffer_.size() == 0);

  // initialize the entry
  rfe_.fsm = ePipelineFsm::kEmpty;
  rfe_.flit = nullptr;
  rfe_.route.clear();
  rfe_.route.link(routingAlgorithm_);

  // no event is set to trigger
  eventTime_ = U64_MAX;
}

InputQueue::~InputQueue() {}

void InputQueue::receiveFlit(u32 _port, Flit* _flit) {
  assert(gSim->epsilon() == 1);

  // 'port' is unused
  assert(_port == 0);

  // make sure this is the right VC (only on head flits)
  //  hyperwarp messes this up for other flits
  assert(!_flit->isHead() || _flit->getVc() == vc_);

  // push flit into corresponding buffer
  buffer_.push(_flit);
  assert(buffer_.size() <= depth_);  // overflow check

  // queue an event to be notified about the injected flit
  //  this synchronized the two clock domains
  if (gSim->isCycle(Simulator::Clock::ROUTER)) {
    setPipelineEvent();
  } else {
    addEvent(gSim->futureCycle(Simulator::Clock::ROUTER, 1),
             1, nullptr, INJECTED_FLIT);
  }
}

void InputQueue::processEvent(void* _event, s32 _type) {
  switch (_type) {
    case (INJECTED_FLIT):
      assert(gSim->epsilon() == 1);
      setPipelineEvent();
      break;

    case (PROCESS_PIPELINE):
      assert(gSim->epsilon() == 3);
      processPipeline();
      break;

    default:
      assert(false);
  }
}

void InputQueue::routingAlgorithmResponse(
    RoutingAlgorithm::Response* _response) {
  assert(rfe_.fsm == ePipelineFsm::kWaitingForResponse);
  rfe_.fsm = ePipelineFsm::kWaitingForTransfer;
  assert(gSim->epsilon() == 0);

  // retrieve the routing algorithm outputs, randomly select one
  u32 routeIndex = gSim->rnd.nextU64(0, rfe_.route.size() - 1);
  u32 outputPort, outputVc;
  rfe_.route.get(routeIndex, &outputPort, &outputVc);

  // inform the routing algorithm of vc scheduled
  routingAlgorithm_->vcScheduled(rfe_.flit, outputPort, outputVc);

  // log traffic
  router_->network()->logTraffic(
      router_, port_, vc_, outputPort, outputVc,
      rfe_.flit->packet()->numFlits());

  // tell the router that this packet wants to access the output queue
  router_->registerPacket(port_, vc_, rfe_.flit, outputPort, outputVc);

  // ensure an event is set to process the pipeline
  setPipelineEvent();
}

void InputQueue::pullPacket(Flit* _headFlit) {
  assert(rfe_.fsm == ePipelineFsm::kWaitingForTransfer);
  assert(_headFlit == rfe_.flit);

  // tell the pipeline this packet can proceed
  rfe_.fsm = ePipelineFsm::kReadyToAdvance;

  // ensure an event is set to process the pipeline
  setPipelineEvent();
}

void InputQueue::setPipelineEvent() {
  if (eventTime_ == U64_MAX) {
    eventTime_ = gSim->time();
    addEvent(gSim->time(), 3, nullptr, PROCESS_PIPELINE);
  }
}

void InputQueue::processPipeline() {
  // make sure the pipeline is being processed on clock cycle boundaries
  assert(gSim->time() % gSim->cycleTime(Simulator::Clock::ROUTER) == 0);

  /*
   * register the packet with the router core and wait for the pull
   */
  if (rfe_.fsm == ePipelineFsm::kReadyToAdvance) {
    // clear the RFE stage
    rfe_.fsm = ePipelineFsm::kEmpty;
    rfe_.flit = nullptr;
    rfe_.route.clear();
  }

  /*
   * attempt to load RFE stage
   */
  if ((rfe_.fsm == ePipelineFsm::kEmpty) && (buffer_.empty() == false)) {
    // ensure RFE is empty
    assert(rfe_.flit == nullptr);

    // pull out the front flit
    Flit* flit = buffer_.front();
    buffer_.pop();

    // send a credit back
    router_->sendCredit(port_, vc_);

    // put it in the routing pipeline stage
    assert(rfe_.flit == nullptr);
    rfe_.flit = flit;

    // set state as ready to request routing algorithm
    rfe_.fsm = ePipelineFsm::kWaitingToRequest;
  }

  /*
   * attempt to submit a routing request
   */
  if (rfe_.fsm == ePipelineFsm::kWaitingToRequest) {
    // if this is a head flit, submit a routing request
    if (rfe_.flit->isHead()) {
      // submit request
      routingAlgorithm_->request(this, rfe_.flit, &rfe_.route);

      // set state machine
      rfe_.fsm = ePipelineFsm::kWaitingForResponse;
    } else {
      // not a head flit, set as ready to advance, queue event for this stage
      rfe_.fsm = ePipelineFsm::kReadyToAdvance;
    }
  }

  // clear the eventTime_ variable to indicate no more events are set
  eventTime_ = U64_MAX;

  /*
   * there are a few reasons that the next cycle should be processed:
   *  1. RFE body flit, made progress, needs to continue
   *  2. more flits in the queue, need to pull one out
   * if any of these cases are true, create an event to handle the next cycle
   */
  if ((rfe_.fsm == ePipelineFsm::kReadyToAdvance) ||    // body flit
      ((buffer_.size() > 0) &&    // more flits in buffer
       (rfe_.fsm == ePipelineFsm::kEmpty))) {  // RFE empty
    // set a pipeline event for the next cycle
    eventTime_ = gSim->futureCycle(Simulator::Clock::ROUTER, 1);
    addEvent(eventTime_, 3, nullptr, PROCESS_PIPELINE);
  }
}

}  // namespace OutputQueued
