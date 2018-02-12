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
#include <factory/FunctionFactory.h>
#include <prim/prim.h>

#include "routing/NonMinimalWeightFunc.h"

f64 regularNonMinimalWeightFunc(
    u32 _minimalHops, u32 _nonMinimalHops,
    f64 _minimalCongestion, f64 _nonMinimalCongestion,
    f64 _congestionBias, f64 _independentBias) {
  (void)_minimalHops;  // unused
  (void)_minimalCongestion;  // unused
  return (_nonMinimalCongestion + _congestionBias) * _nonMinimalHops +
      _independentBias;
}

registerWithFunctionFactory(
    "regular", regularNonMinimalWeightFunc,
    NONMINIMALWEIGHTFUNC_RET, NONMINIMALWEIGHTFUNC_ARGS);
