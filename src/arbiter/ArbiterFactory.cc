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
#include "arbiter/ArbiterFactory.h"

#include <cassert>

#include "arbiter/ComparingArbiter.h"
#include "arbiter/LslpArbiter.h"
#include "arbiter/RandomArbiter.h"
#include "arbiter/RandomPriorityArbiter.h"

Arbiter* ArbiterFactory::createArbiter(
    const std::string& _name, const Component* _parent,
    u32 _size, Json::Value _settings) {
  std::string type = _settings["type"].asString();

  if (type == "lslp") {
    return new LslpArbiter(_name, _parent, _size, _settings);
  } else if (type == "comparing") {
    return new ComparingArbiter(_name, _parent, _size, _settings);
  } else if (type == "random") {
    return new RandomArbiter(_name, _parent, _size, _settings);
  } else if (type == "random_priority") {
    return new RandomPriorityArbiter(_name, _parent, _size, _settings);
  } else {
    fprintf(stderr, "unknown Arbiter 'type': %s\n", type.c_str());
    assert(false);
  }
}
