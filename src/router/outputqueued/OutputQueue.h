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
#ifndef ROUTER_OUTPUTQUEUED_OUTPUTQUEUE_H_
#define ROUTER_OUTPUTQUEUED_OUTPUTQUEUE_H_

#include <prim/prim.h>

#include <string>
#include <queue>
#include <vector>

#include "event/Component.h"
#include "architecture/Crossbar.h"
#include "architecture/CrossbarScheduler.h"
#include "types/Flit.h"

namespace OutputQueued {

class Router;

class OutputQueue : public Component,
                    public CrossbarScheduler::Client {
 public:
  OutputQueue(const std::string& _name, const Component* _parent,
              u32 _port, u32 _vc, CrossbarScheduler* _outputCrossbarScheduler,
              u32 _crossbarSchedulerIndex, Crossbar* _crossbar,
              u32 _crossbarIndex, CreditWatcher* _creditWatcher,
              u32 _creditWatcherVcId, bool _incrCreditWatcher,
              bool _decrCreditWatcher);
  ~OutputQueue();

  // called by router
  void receivePacket(Packet* _packet);

  // event system (Component)
  void processEvent(void* _event, s32 _type) override;

  // response from CrossbarScheduler
  void crossbarSchedulerResponse(u32 _port, u32 _vc) override;

 private:
  void setPipelineEvent();
  void processPipeline();

  // attributes
  const u32 port_;
  const u32 vc_;

  // external devices
  CrossbarScheduler* outputCrossbarScheduler_;
  const u32 crossbarSchedulerIndex_;
  Crossbar* crossbar_;
  const u32 crossbarIndex_;
  CreditWatcher* creditWatcher_;
  const u32 creditWatcherVcId_;
  const bool incrCreditWatcher_;
  const bool decrCreditWatcher_;

  // state machine to represent a generic pipeline stage
  enum class ePipelineFsm { kEmpty, kWaitingToRequest, kWaitingForResponse,
      kReadyToAdvance };

  // remembers if an event is set to process the pipeline
  u64 eventTime_;

  // The following variables represent the pipeline registers

  // buffer
  std::queue<Flit*> buffer_;  // insertion time & inserted flit

  // Switch allocation [swa_] pipeline stage
  struct {
    ePipelineFsm fsm;
    Flit* flit;
  } swa_;

  // Crossbar traversal [xtr_] stage (no state needed)
};

}  // namespace OutputQueued

#endif  // ROUTER_OUTPUTQUEUED_OUTPUTQUEUE_H_
