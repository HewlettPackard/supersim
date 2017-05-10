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
#include "allocator/Allocator_TEST.h"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "allocator/Allocator.h"

#include "test/TestSetup_TEST.h"

u64 AllocatorIndex(u64 _numClients, u64 _client, u64 _resource) {
  return _numClients * _resource + _client;
}

static std::string ppp(bool* _bools, u64 _len) {
  std::stringstream ss;
  ss << '[';
  while (_len > 2) {
    ss << (*_bools ? '1' : '0');
    _len--;
    _bools++;
  }
  ss << (*_bools ? '1' : '0') << ']';
  return ss.str();
}

void AllocatorTest(Json::Value _settings, AllocatorVerifier _verifier,
                   bool _singleRequest) {
  for (u32 C = 1; C < 16; C++) {
    for (u32 R = 1; R < 16; R++) {
      TestSetup testSetup(1, 1, 123);

      // printf("C=%u R=%u\n", C, R);
      bool* request = new bool[C * R];
      bool* requestDup = new bool[C * R];
      u64* metadata = new u64[C * R];
      bool* grant = new bool[C * R];

      // create the allocator
      Allocator* alloc = Allocator::create(
          "Alloc", nullptr, C, R, _settings);

      // map I/O to the allocator
      for (u32 c = 0; c < C; c++) {
        for (u32 r = 0; r < R; r++) {
          u64 idx = AllocatorIndex(C, c, r);
          alloc->setRequest(c, r, &request[idx]);
          alloc->setMetadata(c, r, &metadata[idx]);
          alloc->setGrant(c, r, &grant[idx]);
        }
      }

      // run the test numerous times
      for (u32 run = 0; run < 100; run++) {
        // randomize the inputs
        for (u32 c = 0; c < C; c++) {
          u32 reqCount = 0;
          do {
            u32 single = gSim->rnd.nextU64(0, R);
            for (u32 r = 0; r < R; r++) {
              u64 idx = AllocatorIndex(C, c, r);
              metadata[idx] = 10000 + c;
              if (_singleRequest) {
                request[idx] = idx == single;
              } else {
                request[idx] = gSim->rnd.nextBool();
              }
              requestDup[idx] = request[idx];
              if (request[idx]) {
                reqCount++;
              }
            }
          } while (!_singleRequest && reqCount <= R/3);
        }

        // clear the grants
        for (u32 c = 0; c < C; c++) {
          for (u32 r = 0; r < R; r++) {
            grant[AllocatorIndex(C, c, r)] = false;
          }
        }

        // allocate
        if (false) {
          printf("r %s\n", ppp(request, C*R).c_str());
        }
        alloc->allocate();
        if (false) {
          printf("g %s\n", ppp(grant, C*R).c_str());
        }

        // verify that if something was granted, it was requested
        for (u32 c = 0; c < C; c++) {
          for (u32 r = 0; r < R; r++) {
            u64 idx = AllocatorIndex(C, c, r);
            if (grant[idx]) {
              ASSERT_TRUE(requestDup[idx]);
            }
          }
        }

        // verify
        if (_verifier) {
          _verifier(C, R, request, metadata, grant);
        }

        // reset the requests
        for (u32 c = 0; c < C; c++) {
          for (u32 r = 0; r < R; r++) {
            u64 idx = AllocatorIndex(C, c, r);
            request[idx] = false;
          }
        }
      }

      // cleanup
      delete[] request;
      delete[] requestDup;
      delete[] metadata;
      delete[] grant;
      delete alloc;
    }
  }
}

void AllocatorLoadBalanceTest(Json::Value _settings) {
  const bool DBG = false;
  const u32 C = 16;
  const u32 R = 16;

  TestSetup testSetup(1, 1, 123);

  // printf("C=%u R=%u\n", C, R);
  bool* request = new bool[C * R];
  bool* requestDup = new bool[C * R];
  u64* metadata = new u64[C * R];
  bool* grant = new bool[C * R];
  u32* resourceGrantCounts = new u32[C * R];
  u32* clientGrantCounts = new u32[C];

  // create the allocator
  Allocator* alloc = Allocator::create(
      "Alloc", nullptr, C, R, _settings);

  // map I/O to the allocator
  for (u32 c = 0; c < C; c++) {
    for (u32 r = 0; r < R; r++) {
      u64 idx = AllocatorIndex(C, c, r);
      alloc->setRequest(c, r, &request[idx]);
      alloc->setMetadata(c, r, &metadata[idx]);
      alloc->setGrant(c, r, &grant[idx]);
    }
  }

  // run the test numerous times
  for (u32 test = 0; test < 1; test++) {
    // reset the grant counts
    u64 totalGrants = 0;
    for (u32 c = 0; c < C; c++) {
      clientGrantCounts[c] = 0;
      for (u32 r = 0; r < R; r++) {
        resourceGrantCounts[AllocatorIndex(C, c, r)] = 0;
      }
    }

    // pick random requests for each client
    const u32 reqsPerClient = 8;

    // choose random resources for each client to request
    std::unordered_set<u32> rndRes;
    for (u32 r = 0; r < reqsPerClient; r++) {
      u32 rnd = U32_MAX;
      do {
        rnd = gSim->rnd.nextU64(0, R-1);
      } while (rndRes.count(rnd) > 0);
      bool res = rndRes.insert(rnd).second;
      (void)res;
      assert(res);
    }

    for (u32 c = 0; c < C; c++) {
      // default requests to false
      for (u32 r = 0; r < R; r++) {
        u64 idx = AllocatorIndex(C, c, r);
        metadata[idx] = 10000;
        requestDup[idx] = false;
      }

      // apply random choices
      for (auto r : rndRes) {
        u64 idx = AllocatorIndex(C, c, r);
        assert(requestDup[idx] == false);
        requestDup[idx] = true;
      }
    }

    // run many times
    for (u32 run = 0; run < 100000; run++) {
      // set the requests, clear the grants
      for (u32 c = 0; c < C; c++) {
        for (u32 r = 0; r < R; r++) {
          u64 idx = AllocatorIndex(C, c, r);
          request[idx] = requestDup[idx];
          grant[idx] = false;
        }
      }

      // allocate
      if (DBG) {
        printf("r %s\n", ppp(request, C*R).c_str());
      }
      alloc->allocate();
      if (DBG) {
        printf("g %s\n", ppp(grant, C*R).c_str());
      }

      // verify that if something was granted, it was requested
      //  increment the grant count
      for (u32 c = 0; c < C; c++) {
        u32 rg = U32_MAX;
        for (u32 r = 0; r < R; r++) {
          u64 idx = AllocatorIndex(C, c, r);
          if (grant[idx]) {
            ASSERT_EQ(rg, U32_MAX);
            totalGrants++;
            clientGrantCounts[c]++;
            resourceGrantCounts[idx]++;
            ASSERT_TRUE(requestDup[idx]);
            rg = r;
          }
        }
      }
    }

    // check the grant distribution
    for (u32 c = 0; c < C; c++) {
      if (DBG) {
        printf("\nClient: %u (%.2f%%)\n", clientGrantCounts[c],
               (clientGrantCounts[c] / (f64)totalGrants) * 100.0);
      }
      for (u32 r = 0; r < R; r++) {
        u64 idx = AllocatorIndex(C, c, r);
        if (requestDup[idx]) {
          if (DBG) {
            printf("c=%u r=%u gc=%u\n", c, r, resourceGrantCounts[idx]);
          }
          f64 percent = (f64)resourceGrantCounts[idx] / clientGrantCounts[c];
          f64 expected = 1.0 / reqsPerClient;
          ASSERT_NEAR(percent, expected, 0.01);
        } else {
          ASSERT_EQ(resourceGrantCounts[idx], 0u);
        }
      }
    }
  }

  // cleanup
  delete[] request;
  delete[] requestDup;
  delete[] metadata;
  delete[] grant;
  delete[] resourceGrantCounts;
  delete[] clientGrantCounts;
  delete alloc;
}
