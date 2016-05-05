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
#include "router/common/Crossbar.h"

#include <cassert>

#include "event/Simulator.h"

Crossbar::Crossbar(const std::string& _name, const Component* _parent,
                   u32 _numInputs, u32 _numOutputs,
                   Json::Value _settings)
    : Component(_name, _parent),
      numInputs_(_numInputs), numOutputs_(_numOutputs) {
  latency_ = _settings["latency"].asUInt();  // latency is in cycles
  assert(latency_ > 0);

  std::pair<u32, FlitReceiver*> def(U32_MAX, nullptr);
  receivers_.resize(numOutputs_, def);
  nextTime_ = U64_MAX;
}

Crossbar::~Crossbar() {}

u32 Crossbar::numInputs() const {
  return numInputs_;
}

u32 Crossbar::numOutputs() const {
  return numOutputs_;
}

void Crossbar::setReceiver(u32 _destId, FlitReceiver* _receiver,
                           u32 _destPort) {
  receivers_.at(_destId).first  = _destPort;
  receivers_.at(_destId).second = _receiver;
}

void Crossbar::inject(Flit* _flit, u32 _srcId, u32 _destId) {
  // 'srcId' is not being used, but is available for debugging
  assert(_srcId < numInputs_);

  // determine if this is a new cycle
  u64 nextTime = gSim->futureCycle(1);
  if (nextTime_ != nextTime) {
    nextTime_ = nextTime;

    // push in a new map
    destMaps_.push_front(std::vector<Flit*>(numOutputs_, nullptr));

    // schedule an event for the new map
    addEvent(gSim->futureCycle(latency_), 1, nullptr, 0);
  }

  std::vector<Flit*>& map = destMaps_.front();
  // check to ensure the output has not been double booked
  assert(map.at(_destId) == nullptr);
  // map in the info
  map.at(_destId) = _flit;
}

void Crossbar::processEvent(void* _event, s32 _type) {
  assert(gSim->epsilon() == 1);
  assert(destMaps_.size() >= 1);

  // pull out the next map
  assert(destMaps_.empty() == false);
  std::vector<Flit*>& map = destMaps_.back();

  // send all flits
  for (u32 i = 0; i < map.size(); i++) {
    if (map.at(i)) {
      u32 port = receivers_.at(i).first;
      assert(port != U32_MAX);
      FlitReceiver* receiver = receivers_.at(i).second;
      assert(receiver != nullptr);
      receiver->receiveFlit(port, map.at(i));
    }
  }

  // pop the map
  destMaps_.pop_back();
}
