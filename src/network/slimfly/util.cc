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
#include "network/slimfly/util.h"

#include <cassert>

namespace SlimFly {

//TODO: implement
bool isPrimePower(u32 _width) {
	return true;
}

//TODO: implement
static u32 computePrimitiveElement(u32 _width) {
	return 2;
}

void createGeneratorSet(u32 coeff, int delta, std::vector<u32>& X, std::vector<u32>& X_i) {

	// compute primitive element
	u32 _width = 4 * coeff + delta;
	u32 prim = computePrimitiveElement(_width);
	u32 curr = 1;
	for (u32 i = 0; i < 2*coeff; i++) {
		X.push_back(curr);
		X_i.push_back(curr * prim);
		curr *= prim*prim; 	
		if ((i == coeff) && (delta == -1)) curr /= prim;
	}
}

void computeAddress(u32 _id, u32 _width,
                    u32 _concentration, std::vector<u32>* _address) {
  u32 dimensions = 3; 
  _address->resize(dimensions + 1);

  // addresses are in little endian format
  u32 mod, div;
  mod = _id % _concentration;
  div = _id / _concentration;
  _address->at(0) = mod;
  for (u32 dim = 0; dim < dimensions; dim++) {
    mod = div % _width;
    div = div / _width;
    _address->at(dim + 1) = mod;
  }
}

}  // namespace SlimFly