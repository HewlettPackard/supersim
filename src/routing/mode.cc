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
#include "routing/mode.h"

#include <cassert>

#include <algorithm>

#include "router/Router.h"

f64 averagePortCongestion(const Router* _router, u32 _inputPort, u32 _inputVc,
                          u32 _outputPort) {
  const u32 numVcs = _router->numVcs();
  f64 sum = 0;
  for (u32 vc = 0; vc < numVcs; vc++) {
    sum += _router->congestionStatus(_inputPort, _inputVc, _outputPort, vc);
  }
  return sum / numVcs;
}

f64 minimumPortCongestion(const Router* _router, u32 _inputPort, u32 _inputVc,
                          u32 _outputPort) {
  const u32 numVcs = _router->numVcs();
  f64 minimum = F64_POS_INF;
  for (u32 vc = 0; vc < numVcs; vc++) {
    minimum = std::min(minimum, _router->congestionStatus(_inputPort, _inputVc,
                                                          _outputPort, vc));
  }
  return minimum;
}

f64 maximumPortCongestion(const Router* _router, u32 _inputPort, u32 _inputVc,
                          u32 _outputPort) {
  const u32 numVcs = _router->numVcs();
  f64 maximum = F64_NEG_INF;
  for (u32 vc = 0; vc < numVcs; vc++) {
    maximum = std::max(maximum, _router->congestionStatus(_inputPort, _inputVc,
                                                          _outputPort, vc));
  }
  return maximum;
}

f64 portCongestion(RoutingMode _mode, const Router* _router, u32 _inputPort,
                   u32 _inputVc, u32 _outputPort) {
  switch (_mode) {
    case RoutingMode::kPortAve:
      return averagePortCongestion(_router, _inputPort, _inputVc, _outputPort);
    case RoutingMode::kPortMin:
      return minimumPortCongestion(_router, _inputPort, _inputVc, _outputPort);
    case RoutingMode::kPortMax:
      return maximumPortCongestion(_router, _inputPort, _inputVc, _outputPort);
    default:
      assert(false);
  }
}

RoutingMode parseRoutingMode(const std::string& _mode) {
  if (_mode == "vc") {
    return RoutingMode::kVc;
  } else if (_mode == "port_ave") {
    return RoutingMode::kPortAve;
  } else if (_mode == "port_min") {
    return RoutingMode::kPortMin;
  } else if (_mode == "port_max") {
    return RoutingMode::kPortMax;
  } else {
    fprintf(stderr, "unknown routing mode: '%s'\n", _mode.c_str());
    assert(false);
  }
}

bool routingModeIsPort(RoutingMode _mode) {
  return _mode >= RoutingMode::kPortAve;
}
