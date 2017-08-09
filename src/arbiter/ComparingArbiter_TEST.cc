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
#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "arbiter/ComparingArbiter.h"

#include "arbiter/Arbiter_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(ComparingArbiter, full) {
  TestSetup testSetup(1, 1, 1, 123);

  for (u32 size = 1; size < 100; size++) {
    for (u32 dir = 0; dir < 2; dir++) {
      bool greater = dir == 1;

      bool* request = new bool[size];
      u64* metadata = new u64[size];
      bool* grant = new bool[size];
      do {
        for (u32 idx = 0; idx < size; idx++) {
          request[idx] = gSim->rnd.nextBool();
          metadata[idx] = 10000 + idx;
        }
      } while (hotCount(request, size) <= size/2);
      /*putc('=', stdout);
        for (u32 idx = 0; idx < size; idx++) {
        putc(request[idx].enable ? '1' : '0', stdout);
        }
        putc('\n', stdout);*/

      Json::Value asettings;
      asettings["greater"] = greater;
      Arbiter* arb = new ComparingArbiter("Arb", nullptr, size, asettings);
      assert(arb->size() == size);
      for (u32 idx = 0; idx < size; idx++) {
        arb->setRequest(idx, &request[idx]);
        arb->setMetadata(idx, &metadata[idx]);
        arb->setGrant(idx, &grant[idx]);
      }

      // do more arbitrations
      while (hotCount(request, size) > 0) {
        memset(grant, false, size);
        arb->arbitrate();

        ASSERT_EQ(hotCount(grant, size), 1u);
        u32 awinner = winnerId(grant, size);
        u32 ewinner = U32_MAX;
        u64 emeta = U64_MAX;
        for (u32 idx = 0; idx < size; idx++) {
          if (request[idx]) {
            if (ewinner == U32_MAX) {
              ewinner = idx;
              emeta = metadata[idx];
            } else {
              ASSERT_NE(metadata[idx], emeta);
              if ((greater) && (metadata[idx] > emeta)) {
                ewinner = idx;
                emeta = metadata[idx];
              } else if ((!greater) && (metadata[idx] < emeta)) {
                ewinner = idx;
                emeta = metadata[idx];
              }
            }
          }
        }
        ASSERT_NE(ewinner, U32_MAX);
        ASSERT_NE(emeta, U64_MAX);
        ASSERT_EQ(awinner, ewinner);

        // turn off the winner
        request[awinner] = false;
      }

      // zero requests input test
      ASSERT_EQ(hotCount(request, size), 0u);
      memset(grant, false, size);
      arb->arbitrate();
      ASSERT_EQ(hotCount(grant, size), 0u);

      // cleanup
      delete[] request;
      delete[] metadata;
      delete[] grant;
      delete arb;
    }
  }
}

TEST(ComparingArbiter, dist) {
  for (u8 quads = 1; quads <= 4; quads *= 2) {
    for (u32 size = 32; size <= 64; size += 8) {
      TestSetup testSetup(1, 1, 1, 123);

      bool* request = new bool[size];
      u64* metadata = new u64[size];
      bool* grant = new bool[size];
      u32* count = new u32[size];

      for (u32 idx = 0; idx < size; idx++) {
        u32 quad = idx % quads;
        request[idx] = quad == 0;
        metadata[idx] = 0;
        count[idx] = 0;
      }

      Json::Value arbSettings;
      arbSettings["greater"] = false;
      Arbiter* arb = new ComparingArbiter("Arb", nullptr, size, arbSettings);
      assert(arb->size() == size);
      for (u32 idx = 0; idx < size; idx++) {
        arb->setRequest(idx, &request[idx]);
        arb->setMetadata(idx, &metadata[idx]);
        arb->setGrant(idx, &grant[idx]);
      }

      const u32 ROUNDS = 500000;
      for (u32 round = 0; round < ROUNDS; round++) {
        // reset grants and arbitrate
        memset(grant, false, size);
        arb->arbitrate();

        // check one hot and perform count
        ASSERT_EQ(hotCount(grant, size), 1u);
        u32 awinner = winnerId(grant, size);
        count[awinner]++;
      }

      // verify count distribution
      for (u32 idx = 0; idx < size; idx++) {
        u32 quad = idx % quads;
        if (quad != 0) {
          ASSERT_EQ(count[idx], 0u);
        } else {
          f64 percent = (f64)count[idx] / ROUNDS;
          f64 expected = ((ROUNDS * quads) / (f64)size) / ROUNDS;
          ASSERT_NEAR(percent, expected, 0.001);
        }
      }

      // cleanup
      delete[] request;
      delete[] metadata;
      delete[] grant;
      delete[] count;
      delete arb;
    }
  }
}
