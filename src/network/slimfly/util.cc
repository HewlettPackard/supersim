/*
 * Copyright 2016 Ashish Chaudhari, Franky Romero, Nehal Bhandari, Wasam Altoyan
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
#include <math.h>
#include <cassert>

namespace SlimFly {

// TODO(Nehal): implement
bool isPrimePower(u32 _width) {
  return true;
}

static bool isPrimitiveElement(u32 _width, u32 prim) {
  std::vector<bool> satisfy(_width, false);
  satisfy[0] = true;  // exception for primitive elem
  for (u32 p = 0; p < _width; p++) {
    satisfy[static_cast<u32>(pow(prim, p)) % _width] = true;
  }
  bool isprim = true;
  for (bool s : satisfy) {
    isprim &= s;
  }
  return isprim;
}

u32 createGeneratorSet(
    u32 _width, int delta, std::vector<u32>& X, std::vector<u32>& X_i) {
  u32 prim = 1;
  for (prim = 1; prim < _width; prim++) {
    if (isPrimitiveElement(_width, prim))
      break;
  }
  assert(prim < _width);
  u32 last_pow = (delta == 1) ? _width - 3 : _width - 2;
  X.clear();
  X_i.clear();
  for (u32 p = 0; p <= last_pow; p += 2) {
    if ((delta == -1) && p == (_width + 1) / 2) {
      p--;
    }
    u32 val = pow(prim, p);
    X.push_back(val % _width);
    X_i.push_back((val * prim) % _width);
  }
  return X.size();
}

void addressFromInterfaceId(u32 _id, u32 _width,
    u32 _concentration, std::vector<u32>* _address) {
  _address->clear();
  _address->resize(4);  // Dimwidth is always 3 so 4 = 3 + 1

  u32 curr = _id;
  (*_address)[0] = curr % _concentration;
  curr = curr / _concentration;
  (*_address)[3] = curr % _width;
  curr = curr / _width;
  (*_address)[2] = curr % _width;
  curr = curr / _width;
  (*_address)[1] = curr;

  assert(_id == ifaceIdFromAddress(*_address, _width, _concentration));
}

u32 ifaceIdFromAddress(const std::vector<u32>& _address,
    u32 _width, u32 _concentration) {
  return (
      _address[0] +
      (_concentration * _address[3]) +
      (_concentration * _width * _address[2]) +
      (_concentration * _width * _width * _address[1]));
}

}  // namespace SlimFly
