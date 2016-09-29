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
#ifndef APPLICATION_MESSENGER_H_
#define APPLICATION_MESSENGER_H_

#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "stats/RateLog.h"

class Application;
class InjectionLimiter;
class EnterNotifier;
class ExitNotifier;

class MessageReceiver;
class Interface;
class Terminal;

class Messenger : public Component {
 public:
  Messenger(const std::string& _name, const Component* _parent,
            Application* _app, u32 _id, f64 _maxInjectionRate);
  ~Messenger();
  void linkInterface(Interface* _interface);
  void linkTerminal(Terminal* _terminal);

 private:
  Application* app_;
  u32 id_;
  InjectionLimiter* injectionLimiter_;
  EnterNotifier* enterNotifier_;
  ExitNotifier* exitNotifier_;
};

#endif  // APPLICATION_MESSENGER_H_
