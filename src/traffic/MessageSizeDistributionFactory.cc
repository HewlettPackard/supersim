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
#include "traffic/MessageSizeDistributionFactory.h"

#include <cassert>

#include "traffic/ProbabilityMessageSizeDistribution.h"
#include "traffic/RandomMessageSizeDistribution.h"
#include "traffic/SingleMessageSizeDistribution.h"

MessageSizeDistribution*
MessageSizeDistributionFactory::createMessageSizeDistribution(
    const std::string& _name, const Component* _parent, Json::Value _settings) {
  const std::string& type = _settings["type"].asString();

  if (type == "single") {
    return new SingleMessageSizeDistribution(
        _name, _parent, _settings);
  } else if (type == "random") {
    return new RandomMessageSizeDistribution(
        _name, _parent, _settings);
  } else if (type == "probability") {
    return new ProbabilityMessageSizeDistribution(
        _name, _parent, _settings);
  } else {
    fprintf(stderr, "unknown message size distribution: %s\n", type.c_str());
    assert(false);
  }
}
