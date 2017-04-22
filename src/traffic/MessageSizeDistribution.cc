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
#include "traffic/MessageSizeDistribution.h"

#include <factory/Factory.h>

MessageSizeDistribution::MessageSizeDistribution(
    const std::string& _name, const Component* _parent, Json::Value _settings)
    : Component(_name, _parent) {}

MessageSizeDistribution::~MessageSizeDistribution() {}

MessageSizeDistribution* MessageSizeDistribution::create(
    const std::string& _name, const Component* _parent, Json::Value _settings) {
  // retrieve the type
  const std::string& type = _settings["type"].asString();

  // attempt to build the message size distribution
  MessageSizeDistribution* msg = factory::Factory<
    MessageSizeDistribution, MESSAGESIZEDISTRIBUTION_ARGS>::create(
        type, _name, _parent, _settings);

  // check that the factory had this type
  if (msg == nullptr) {
    fprintf(stderr, "unknown message size distribution type: %s\n",
            type.c_str());
    assert(false);
  }
  return msg;
}
