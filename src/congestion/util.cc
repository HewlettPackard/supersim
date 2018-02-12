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
#include "congestion/util.h"

#include <cmath>

bool congestionEqualTo(f64 _cong1, f64 _cong2) {
  return std::abs(_cong1 - _cong2) < CONGESTION_TOLERANCE;
}

bool congestionLessThan(f64 _cong1, f64 _cong2) {
  return (_cong2 - _cong1) >= CONGESTION_TOLERANCE;
}

bool congestionGreaterThan(f64 _cong1, f64 _cong2) {
  return (_cong1 - _cong2) >= CONGESTION_TOLERANCE;
}
