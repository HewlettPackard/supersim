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
#ifndef WORKLOAD_INJECTIONLIMITER_H_
#define WORKLOAD_INJECTIONLIMITER_H_

#include <json/json.h>
#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"

class Application;

class InjectionLimiter : public Component, public MessageReceiver {
 public:
  InjectionLimiter(const std::string& _name, const Component* _parent,
                   Application* _app, u32 _id);
  ~InjectionLimiter();

  void setMessageReceiver(MessageReceiver* _receiver);
  void receiveMessage(Message* _message) override;
  void processEvent(void* _event, s32 _type) override;

 private:
  Application* app_;
  u32 id_;
  MessageReceiver* receiver_;

  bool enabled_;
  u64 lastTime_;
};

#endif  // WORKLOAD_INJECTIONLIMITER_H_
