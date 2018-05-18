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
#ifndef ARCHITECTURE_UTIL_H_
#define ARCHITECTURE_UTIL_H_

#include <prim/prim.h>

#include <vector>

u32 computeTailoredBufferLength(f64 _inputQueueMult, u32 _inputQueueMin,
                                u32 _inputQueueMax, u32 _channelLatency);

#endif  // ARCHITECTURE_UTIL_H_
