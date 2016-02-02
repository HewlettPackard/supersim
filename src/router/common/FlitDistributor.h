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
#ifndef ROUTER_COMMON_FLITDISTRIBUTOR_H_
#define ROUTER_COMMON_FLITDISTRIBUTOR_H_

#include <prim/prim.h>

#include <string>
#include <vector>

#include "event/Component.h"
#include "types/Flit.h"
#include "types/FlitReceiver.h"

class FlitDistributor : public Component, public FlitReceiver {
 public:
  FlitDistributor(const std::string& _name, const Component* _parent,
                  u32 _outputs);
  ~FlitDistributor();

  void setReceiver(u32 _vc, FlitReceiver* _receiver);

  void receiveFlit(u32 _port, Flit* _flit) override;

 private:
  std::vector<FlitReceiver*> receivers_;
};

#endif  // ROUTER_COMMON_FLITDISTRIBUTOR_H_
