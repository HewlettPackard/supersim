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
#ifndef EVENT_VECTORQUEUE_H_
#define EVENT_VECTORQUEUE_H_

#include <jsoncpp/json/json.h>
#include <prim/prim.h>

#include <queue>
#include <vector>

#include "event/Simulator.h"
#include "event/Component.h"

class VectorQueue : public Simulator {
 public:
  explicit VectorQueue(Json::Value _settings);
  ~VectorQueue();
  void addEvent(u64 _time, u8 _epsilon, Component* _component, void* _event,
                s32 _type) override;
  u64 queueSize() const override;

 protected:
  void runNextEvent() override;

 private:
  class EventBundle {
   public:
    u64 time;
    u8 epsilon;
    Component* component;
    void* event;
    s32 type;
  };

  class EventBundleComparator {
   public:
    EventBundleComparator();
    ~EventBundleComparator();
    bool operator()(const EventBundle _lhs, const EventBundle _rhs) const;
  };

  std::priority_queue<EventBundle, std::vector<EventBundle>,
                      EventBundleComparator> eventQueue_;
};

#endif  // EVENT_VECTORQUEUE_H_
