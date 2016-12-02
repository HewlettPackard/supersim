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
#include <json/json.h>
#include <gtest/gtest.h>
#include <prim/prim.h>

#include <string>
#include <vector>

#include "arbiter/Arbiter.h"
#include "arbiter/LruArbiter.h"

#include "arbiter/Arbiter_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(LruArbiter, full) {
  TestSetup testSetup(1, 1, 123);

  for (u32 size = 1; size < 100; size++) {
    bool* request = new bool[size];
    u64* metadata = new u64[size];
    bool* grant = new bool[size];

    Arbiter* arb = new LruArbiter("Arb", nullptr, size, Json::Value());
    assert(arb->size() == size);

    // link the structures
    for (u32 idx = 0; idx < size; idx++) {
      arb->setRequest(idx, &request[idx]);
      arb->setMetadata(idx, &metadata[idx]);
      arb->setGrant(idx, &grant[idx]);
    }

    // start the process will all requesting
    for (u32 idx = 0; idx < size; idx++) {
      request[idx] = true;
      metadata[idx] = 0;
    }

    // these data structures hold the unit tests version of LRU
    std::list<u32> lru;
    std::set<u32> unique;

    // first arbitrations to over come random initialization
    for (u32 idx = 0; idx < size; idx++) {
      memset(grant, false, size);
      arb->arbitrate();
      ASSERT_EQ(hotCount(grant, size), 1u);
      u32 winner = winnerId(grant, size);
      arb->latch();

      // update the expected LRU structure
      lru.push_back(winner);

      // make sure everyone gets serviced once
      bool res = unique.insert(winner).second;
      ASSERT_TRUE(res);
    }
    ASSERT_EQ(lru.size(), size);
    ASSERT_EQ(unique.size(), size);

    // now make a random request vector for testing
    do {
      for (u32 idx = 0; idx < size; idx++) {
        request[idx] = gSim->rnd.nextBool();
        metadata[idx] = 0;
      }
    } while (hotCount(request, size) <= size/2);

    // do more arbitrations
    const u32 ARBS = 100;
    for (u32 a = 0; a < ARBS; a++) {
      memset(grant, false, size);
      arb->arbitrate();

      // verify one-hot and get winner
      ASSERT_EQ(hotCount(grant, size), 1u);
      u32 winner = winnerId(grant, size);

      // compute expected winner
      u32 ewinner = U32_MAX;
      std::list<u32>::iterator ewinnerIt;
      for (auto it = lru.begin(); it != lru.end(); ++it) {
        u32 idx = *it;
        if (request[idx]) {
          ewinner = idx;
          ewinnerIt = it;
          break;
        }
      }
      assert(ewinner != U32_MAX);

      // verify correct operation
      ASSERT_EQ(winner, ewinner);
      ASSERT_TRUE(request[winner]);

      // only latch the priority sometimes
      if (gSim->rnd.nextBool()) {
        arb->latch();

        // rotate the LRU structure
        lru.erase(ewinnerIt);
        lru.push_back(ewinner);
      }
    }

    // zero requests input test
    for (u32 idx = 0; idx < size; idx++) {
      request[idx] = false;
    }
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

TEST(LruArbiter, dist) {
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

      Json::Value arbSettings;
      Arbiter* arb = new LruArbiter("Arb", nullptr, size, arbSettings);
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

        // latch result
        arb->latch();
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
