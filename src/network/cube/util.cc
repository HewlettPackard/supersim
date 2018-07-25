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
#include "network/cube/util.h"

#include <cassert>

namespace Cube {

void translateInterfaceIdToAddress(u32 _id, const std::vector<u32>& _widths,
                            u32 _concentration, std::vector<u32>* _address) {
  u32 dimensions = _widths.size();
  _address->resize(dimensions + 1);

  // addresses are in little endian format
  u32 mod, div;
  mod = _id % _concentration;
  div = _id / _concentration;
  _address->at(0) = mod;
  for (u32 dim = 0; dim < dimensions; dim++) {
    u32 dimWidth = _widths.at(dim);
    mod = div % dimWidth;
    div = div / dimWidth;
    _address->at(dim + 1) = mod;
  }
}

u32 translateInterfaceAddressToId(const std::vector<u32>* _address,
                      const std::vector<u32>& _widths,
                      u32 _concentration) {
  u32 dimensions = _widths.size();
  std::vector<u32> coeff(dimensions + 1);

  // compute coefficients
  u32 prod = 1;
  for (u32 idx = 0; idx < dimensions + 1; idx++) {
    coeff.at(idx) = prod;
    if (idx == 0) {
      assert(_address->at(idx) < _concentration);
      prod *= _concentration;
    } else {
      assert(_address->at(idx) < _widths.at(idx - 1));
      prod *= _widths.at(idx - 1);
    }
  }

  // compute dot product
  u32 sum = 0;
  for (u32 idx = 0; idx < dimensions + 1; idx++) {
    sum += coeff.at(idx) * _address->at(idx);
  }

  return sum;
}

void translateRouterIdToAddress(const u32 _id, const std::vector<u32>& _widths,
                          std::vector<u32>* _address) {
  u32 dimensions = _widths.size();
  _address->resize(dimensions);

  // addresses are in little endian format
  u32 mod, div;
  div = _id;
  for (u32 dim = 0; dim < dimensions; dim++) {
    u32 dimWidth = _widths.at(dim);
    mod = div % dimWidth;
    div = div / dimWidth;
    _address->at(dim) = mod;
  }
}

u32 translateRouterAddressToId(const std::vector<u32>* _address,
                    const std::vector<u32>& _widths) {
  u32 dimensions = _widths.size();
  std::vector<u32> coeff(dimensions);

  // compute coefficients
  u32 prod = 1;
  for (u32 idx = 0; idx < dimensions; idx++) {
    coeff.at(idx) = prod;
    assert(_address->at(idx) < _widths.at(idx));
    prod *= _widths.at(idx);
  }

  // compute dot product
  u32 sum = 0;
  for (u32 idx = 0; idx < dimensions; idx++) {
    sum += coeff.at(idx) * _address->at(idx);
  }

  return sum;
}

}  // namespace Cube
