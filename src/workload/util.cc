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
#include "workload/util.h"

#include <cassert>

#include "event/Simulator.h"

u64 transactionId(u32 _appId, u32 _termId, u32 _msgId) {
  return ((u64)_appId << 56) | ((u64)_termId << 32) | ((u64)_msgId);
}

u32 appId(u64 _transId) {
  return (u32)(_transId >> 56);
}

u64 cyclesToSend(f64 _injectionRate, u32 _numFlits) {
  if (std::isinf(_injectionRate)) {
    return 0;  // infinite injection rate
  }
  assert(_injectionRate > 0.0);
  f64 cycles = _numFlits / _injectionRate;

  // if the number of cycles is not even, probablistic cycles must be computed
  f64 fraction = modf(cycles, &cycles);
  if (fraction != 0.0) {
    assert(fraction > 0.0);
    assert(fraction < 1.0);
    f64 rnd = gSim->rnd.nextF64();
    if (fraction > rnd) {
      cycles += 1.0;
    }
  }
  return (u64)cycles;
}
