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
#ifndef INTERFACE_STANDARD_INPUTQUEUE_H_
#define INTERFACE_STANDARD_INPUTQUEUE_H_

#include <prim/prim.h>

#include <string>
#include <queue>

#include "event/Component.h"
#include "types/Flit.h"
#include "router/common/Crossbar.h"
#include "router/common/CrossbarScheduler.h"

namespace Standard {

class Interface;

class InputQueue : public Component, public CrossbarScheduler::Client {
 public:
  InputQueue(const std::string& _name, Interface* _interface,
             CrossbarScheduler* _crossbarScheduler, u32 _crossbarSchedulerIndex,
             Crossbar* _crossbar, u32 _crossbarIndex, u32 _vc);
  ~InputQueue();

  void receiveFlit(Flit* _flit);
  void crossbarSchedulerResponse(u32 _port, u32 _vc) override;

  void processEvent(void* _event, s32 _type) override;

 private:
  CrossbarScheduler* crossbarScheduler_;
  u32 crossbarSchedulerIndex_;
  Crossbar* crossbar_;
  u32 crossbarIndex_;
  Interface* interface_;
  u32 vc_;

  std::queue<Flit*> stageQueue_;
  std::queue<Flit*> sendQueue_;
};

}  // namespace Standard

#endif  // INTERFACE_STANDARD_INPUTQUEUE_H_
