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
#ifndef ARCHITECTURE_CROSSBAR_H_
#define ARCHITECTURE_CROSSBAR_H_

#include <json/json.h>
#include <prim/prim.h>

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

class Crossbar : public Component {
 public:
  Crossbar(const std::string& _name, const Component* _parent,
           u32 _numInputs, u32 _numOutputs, Simulator::Clock _clock,
           Json::Value _settings);
  ~Crossbar();
  u32 numInputs() const;
  u32 numOutputs() const;
  void setReceiver(u32 _destId, FlitReceiver* _receiver, u32 _destPort);
  // call multiple times for multicast
  void inject(Flit* _flit, u32 _srcId, u32 _destId);
  void processEvent(void* _event, s32 _type) override;

 private:
  const Simulator::Clock clock_;
  const u64 latency_;
  const u32 numInputs_;
  const u32 numOutputs_;
  std::vector<std::pair<u32, FlitReceiver*> > receivers_;
  u64 nextTime_;
  std::list<std::vector<Flit*> > destMaps_;
};

#endif  // ARCHITECTURE_CROSSBAR_H_
