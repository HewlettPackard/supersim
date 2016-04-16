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
#ifndef ROUTER_INPUTOUTPUTQUEUED_INPUTQUEUE_H_
#define ROUTER_INPUTOUTPUTQUEUED_INPUTQUEUE_H_

#include <prim/prim.h>

#include <queue>
#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "router/common/Crossbar.h"
#include "router/common/CrossbarScheduler.h"
#include "router/common/VcScheduler.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

namespace InputOutputQueued {

class Router;

class InputQueue : public Component, public FlitReceiver,
                   public RoutingAlgorithm::Client,
                   public VcScheduler::Client,
                   public CrossbarScheduler::Client {
 public:
  InputQueue(const std::string& _name, const Component* _parent,
             Router* _router, u32 _depth, u32 _port, u32 _numVcs, u32 _vc,
             bool _vcaSwaWait, RoutingAlgorithm* _routingAlgorithm,
             VcScheduler* _vcScheduler, u32 _vcSchedulerIndex,
             CrossbarScheduler* _crossbarScheduler, u32 _crossbarSchedulerIndex,
             Crossbar* _crossbar, u32 _crossbarIndex);
  ~InputQueue();

  // called by next higher router (FlitReceiver)
  void receiveFlit(u32 _port, Flit* _flit) override;

  // event system (Component)
  void processEvent(void* _event, s32 _type) override;

  // response from routing algorithm
  void routingAlgorithmResponse(RoutingAlgorithm::Response* _response) override;

  // response from VcScheduler
  void vcSchedulerResponse(u32 _vcIdx) override;

  // response from CrossbarScheduler
  void crossbarSchedulerResponse(u32 _port, u32 _vcIdx) override;

 private:
  void setPipelineEvent();
  void processPipeline();

  // attributes
  const u32 depth_;
  const u32 port_;
  const u32 numVcs_;  // in system, not this module
  const u32 vc_;

  // settings
  const bool vcaSwaWait_;  // stall VCA until SWA is empty

  // external devices
  Router* router_;
  RoutingAlgorithm* routingAlgorithm_;
  VcScheduler* vcScheduler_;
  u32 vcSchedulerIndex_;
  CrossbarScheduler* crossbarScheduler_;
  u32 crossbarSchedulerIndex_;
  Crossbar* crossbar_;
  u32 crossbarIndex_;

  // single flit per clock input limit assurance
  u64 lastReceivedTime_;

  // state machine to represent a generic pipeline stage
  enum class ePipelineFsm { kEmpty, kWaitingToRequest, kWaitingForResponse,
      kReadyToAdvance };

  // remembers if an event is set to process the pipeline
  u64 eventTime_;

  // The following variables represent the pipeline registers

  // buffer
  std::queue<Flit*> buffer_;

  // routing algorithm execution [rfe_] pipeline stage
  struct {
    ePipelineFsm fsm;
    Flit* flit;
    // results
    RoutingAlgorithm::Response route;
  } rfe_;

  // VC allocation [vca] pipeline stage
  struct {
    ePipelineFsm fsm;
    Flit* flit;
    RoutingAlgorithm::Response route;
    // results
    u32 allocatedVcIdx;
    u32 allocatedPort;
    u32 allocatedVc;
  } vca_;

  // Switch allocation [swa_] pipeline stage
  struct {
    ePipelineFsm fsm;
    Flit* flit;
    u32 allocatedPort;
    u32 allocatedVcIdx;
  } swa_;

  // Crossbar traversal [xtr_] stage (no state needed)
};

}  // namespace InputOutputQueued

#endif  // ROUTER_INPUTOUTPUTQUEUED_INPUTQUEUE_H_
