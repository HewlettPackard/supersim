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
#include "event/VectorQueue.h"

#include <cassert>

VectorQueue::VectorQueue(Json::Value _settings)
    : Simulator(_settings) {}

VectorQueue::~VectorQueue() {}

void VectorQueue::addEvent(u64 _time, u8 _epsilon, Component* _component,
                           void* _event, s32 _type) {
  assert((_time > time_) ||  // future by time
         ((_time == time_) && (_epsilon > epsilon_)) ||  // future by epsilon
         (initial()));  // has not yet run

  // create a bundle object
  VectorQueue::EventBundle bundle;
  bundle.time      = _time;
  bundle.epsilon   = _epsilon;
  bundle.component = _component;
  bundle.event     = _event;
  bundle.type      = _type;

  // push into queue
  eventQueue_.push(bundle);
}

u64 VectorQueue::queueSize() const {
  return eventQueue_.size();
}

void VectorQueue::runNextEvent() {
  // if there is an event to run, run it
  if (eventQueue_.size() > 0) {
    // process the next event
    VectorQueue::EventBundle bundle = eventQueue_.top();
    time_ = bundle.time;
    epsilon_ = bundle.epsilon;
    bundle.component->processEvent(bundle.event, bundle.type);
    eventQueue_.pop();
  }

  // set the quit_ status
  quit_ = eventQueue_.size() < 1;
}

/** EventBundleComparator sub-class **/
VectorQueue::EventBundleComparator::EventBundleComparator() {}

VectorQueue::EventBundleComparator::~EventBundleComparator() {}

bool VectorQueue::EventBundleComparator::operator()(
    const VectorQueue::EventBundle _lhs,
    const VectorQueue::EventBundle _rhs) const {
  return (_lhs.time == _rhs.time) ?
      (_lhs.epsilon > _rhs.epsilon) :
      (_lhs.time > _rhs.time);
}
