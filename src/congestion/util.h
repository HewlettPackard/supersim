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
#ifndef CONGESTION_UTIL_H_
#define CONGESTION_UTIL_H_

#include <prim/prim.h>

/*
 * Tolerance is used as a slush factor. Differences less than the tolerance are
 *  treated equal. A difference equal to the tolerance is considered
 *  significantly different.
 */
const f64 CONGESTION_TOLERANCE = 1e-6;
bool congestionEqualTo(f64 _cong1, f64 _cong2);
bool congestionLessThan(f64 _cong1, f64 _cong2);
bool congestionGreaterThan(f64 _cong1, f64 _cong2);


#endif  // CONGESTION_UTIL_H_
