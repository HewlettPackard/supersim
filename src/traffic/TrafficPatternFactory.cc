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
#include "traffic/TrafficPatternFactory.h"

#include <cassert>

#include "traffic/RandomExchangeTrafficPattern.h"
#include "traffic/BitComplementTrafficPattern.h"
#include "traffic/BitReverseTrafficPattern.h"
#include "traffic/BitRotateTrafficPattern.h"
#include "traffic/LoopbackTrafficPattern.h"
#include "traffic/ScanTrafficPattern.h"
#include "traffic/TornadoTrafficPattern.h"
#include "traffic/UniformRandomTrafficPattern.h"

TrafficPattern* TrafficPatternFactory::createTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings) {
  const std::string& type = _settings["type"].asString();

  if (type == "loopback") {
    return new LoopbackTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "scan") {
    return new ScanTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else   if (type == "uniform_random") {
    return new UniformRandomTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "bit_complement") {
    return new BitComplementTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "bit_reverse") {
    return new BitReverseTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "bit_rotate") {
    return new BitRotateTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "tornado") {
    return new TornadoTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else if (type == "random_exchange") {
    return new RandomExchangeTrafficPattern(
        _name, _parent, _numTerminals, _self, _settings);
  } else {
    fprintf(stderr, "unknown traffic pattern: %s\n", type.c_str());
    assert(false);
  }
}
