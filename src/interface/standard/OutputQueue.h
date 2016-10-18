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
#ifndef INTERFACE_STANDARD_OUTPUTQUEUE_H_
#define INTERFACE_STANDARD_OUTPUTQUEUE_H_

#include <prim/prim.h>

#include <queue>
#include <string>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"
#include "architecture/Crossbar.h"
#include "architecture/CrossbarScheduler.h"

namespace Standard {

class Interface;

class OutputQueue : public Component, public FlitReceiver,
                    public CrossbarScheduler::Client {
 public:
  OutputQueue(const std::string& _name, Interface* _interface,
              CrossbarScheduler* _crossbarScheduler,
              u32 _crossbarSchedulerIndex, Crossbar* _crossbar,
              u32 _crossbarIndex, u32 _vc);
  ~OutputQueue();

  // called by Interface injection logic
  void receiveFlit(u32 _port, Flit* _flit) override;

  // event system (Component)
  void processEvent(void* _event, s32 _type) override;

  // response from CrossbarScheduler
  void crossbarSchedulerResponse(u32 _port, u32 _vc) override;

 private:
  void setPipelineEvent();
  void processPipeline();

  // attributes
  const u32 vc_;

  // external devices
  CrossbarScheduler* crossbarScheduler_;
  const u32 crossbarSchedulerIndex_;
  Crossbar* crossbar_;
  const u32 crossbarIndex_;
  Interface* interface_;

  // single flit per clock input limit assurance
  u64 lastReceivedTime_;

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

}  // namespace Standard

#endif  // INTERFACE_STANDARD_OUTPUTQUEUE_H_
