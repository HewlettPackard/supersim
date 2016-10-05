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
#ifndef ROUTER_INPUTQUEUED_OUTPUTQUEUE_H_
#define ROUTER_INPUTQUEUED_OUTPUTQUEUE_H_

#include <prim/prim.h>

#include <string>
#include <queue>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

namespace InputQueued {

class Router;

class OutputQueue : public Component, public FlitReceiver {
 public:
  OutputQueue(const std::string& _name, const Component* _parent,
              Router* _router, u32 _depth, u32 _port);
  ~OutputQueue();

  // called by main router crossbar
  void receiveFlit(u32 _port, Flit* _flit) override;

  // event system (Component)
  void processEvent(void* _event, s32 _type) override;

 private:
  void processPipeline();

  // attributes
  const u32 depth_;
  const u32 port_;

  // external components
  Router* router_;

  // single flit per clock input limit assurance
  u64 lastReceivedTime_;

  // state machine to represent a generic pipeline stage
  enum class ePipelineFsm { kEmpty, kWaitingToRequest, kWaitingForResponse,
      kReadyToAdvance };

  // remembers if an event is set to process the pipeline
  u64 eventTime_;

  // buffer
  std::queue<Flit*> buffer_;  // insertion time & inserted flit
};

}  // namespace InputQueued

#endif  // ROUTER_INPUTQUEUED_OUTPUTQUEUE_H_
