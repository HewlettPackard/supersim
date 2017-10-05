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
#ifndef ROUTER_OUTPUTQUEUED_INPUTQUEUE_H_
#define ROUTER_OUTPUTQUEUED_INPUTQUEUE_H_

#include <prim/prim.h>

#include <queue>
#include <string>
#include <vector>

#include "event/Component.h"
#include "network/RoutingAlgorithm.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

namespace OutputQueued {

class Router;

class InputQueue : public Component, public FlitReceiver,
                   public RoutingAlgorithm::Client {
 public:
  InputQueue(const std::string& _name, const Component* _parent,
             Router* _router, u32 _depth, u32 _port, u32 _numVcs, u32 _vc,
             RoutingAlgorithm* _routingAlgorithm);
  ~InputQueue();

  // called by next higher router (FlitReceiver)
  void receiveFlit(u32 _port, Flit* _flit) override;

  // event system (Component)
  void processEvent(void* _event, s32 _type) override;

  // response from routing algorithm
  void routingAlgorithmResponse(RoutingAlgorithm::Response* _response) override;

  // the router calls this to pull the next packet from this queue
  void pullPacket(Flit* _headFlit);

 private:
  void setPipelineEvent();
  void processPipeline();

  // attributes
  const u32 depth_;
  const u32 port_;
  const u32 numVcs_;  // in system, not this module
  const u32 vc_;

  // external devices
  Router* router_;
  RoutingAlgorithm* routingAlgorithm_;

  // state machine to represent the single RFE stage
  enum class ePipelineFsm { kEmpty, kWaitingToRequest, kWaitingForResponse,
      kWaitingForTransfer, kReadyToAdvance };

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
};

}  // namespace OutputQueued

#endif  // ROUTER_OUTPUTQUEUED_INPUTQUEUE_H_
