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
#include "arbiter/RandomArbiter.h"

#include "arbiter/Arbiter_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(RandomArbiter, full) {
  TestSetup testSetup(1, 1, 123);

  for (u32 size = 1; size < 100; size++) {
    bool* request = new bool[size];
    u64* metadata = new u64[size];
    bool* grant = new bool[size];
    do {
      for (u32 idx = 0; idx < size; idx++) {
        request[idx] = gSim->rnd.nextBool();
        metadata[idx] = 0;
      }
    } while (hotCount(request, size) <= size/2);
    /*putc('=', stdout);
      for (u32 idx = 0; idx < size; idx++) {
      putc(request[idx] ? '1' : '0', stdout);
      }
      putc('\n', stdout);*/

    Arbiter* arb = new RandomArbiter(
        "Arb", nullptr, size, Json::Value());
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
      (void)awinner;  // can't predict this for testing

      // turn off the winner
      ASSERT_TRUE(request[awinner]);
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

TEST(RandomArbiter, dist) {
  for (u8 quads = 1; quads <= 4; quads *= 2) {
    for (u32 size = 32; size <= 64; size += 8) {
      TestSetup testSetup(1, 1, 123);

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

      Arbiter* arb = new RandomArbiter(
          "Arb", nullptr, size, Json::Value());
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
