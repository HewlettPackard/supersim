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
#ifndef APPLICATION_RATEMONITOR_H_
#define APPLICATION_RATEMONITOR_H_

#include <prim/prim.h>

#include <string>

#include "event/Component.h"
#include "types/Message.h"
#include "types/MessageReceiver.h"

class RateMonitor : public Component {
 public:
  RateMonitor(const std::string& _name, const Component* _parent);
  ~RateMonitor();

  void monitorMessage(const Message* _message);
  void start();
  void end();
  f64 rate() const;

 private:
  u64 flitCount_;

  u64 startTime_;
  u64 endTime_;

  bool running_;
};

#endif  // APPLICATION_RATEMONITOR_H_
