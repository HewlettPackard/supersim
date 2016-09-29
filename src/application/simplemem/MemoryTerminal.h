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
#ifndef APPLICATION_SIMPLEMEM_MEMORYTERMINAL_H_
#define APPLICATION_SIMPLEMEM_MEMORYTERMINAL_H_

#include <json/json.h>
#include <prim/prim.h>

#include <queue>
#include <string>
#include <vector>

#include "event/Component.h"
#include "application/Terminal.h"

class Application;

namespace SimpleMem {

class MemoryTerminal : public Terminal {
 public:
  MemoryTerminal(const std::string& _name, const Component* _parent,
                 u32 _id, const std::vector<u32>& _address, u32 _memorySlice,
                 ::Application* _app, Json::Value _settings);
  ~MemoryTerminal();
  void processEvent(void* _event, s32 _type) override;
  void receiveMessage(Message* _message) override;
  void messageEnteredInterface(Message* _message) override;
  void messageExitedNetwork(Message* _message) override;

 private:
  enum class eState {kWaiting, kAccessing};

  void startMemoryAccess();
  void sendMemoryResponse();

  u32 memoryOffset_;
  u8* memory_;
  u32 latency_;

  std::queue<Message*> messages_;
  eState fsm_;
};

}  // namespace SimpleMem

#endif  // APPLICATION_SIMPLEMEM_MEMORYTERMINAL_H_
