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
#include "congestion/BufferOccupancy.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include "congestion/CongestionStatus.h"
#include "congestion/CongestionStatus_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(BufferOccupancy, statusCheck_VcMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router("Router", nullptr, 0, {}, numPorts, numVcs,
                              nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "vc";
  BufferOccupancy status("CongestionStatus", &router, &router,
                         statusSettings);
  status.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      status.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &status, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &status);
  check.setDebug(debug);

  u64 time = 1000;
  s32 type = CongestionStatus::DECR;
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, type);

        // advance time
        time++;
      }
    }
  }

  time = 1000000;
  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      // check credits
      u32 max = port * 10 + vc + 2;
      f64 exp = 2.0 / (f64)max;
      check.setEvent(time, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}

TEST(BufferOccupancy, statusCheck_PortMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router("Router", nullptr, 0, {}, numPorts, numVcs,
                              nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "port";
  BufferOccupancy status("CongestionStatus", &router, &router,
                         statusSettings);
  status.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      status.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &status, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &status);
  check.setDebug(debug);

  u64 time = 1000;
  s32 type = CongestionStatus::DECR;
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, type);

        // advance time
        time++;
      }
    }
  }

  time = 1000000;
  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      // check credits
      u32 curSum = 0;
      u32 maxSum = 0;
      for (u32 vc2 = 0; vc2 < numVcs; vc2++) {
        u32 max = port * 10 + vc2 + 2;
        curSum += 2;
        maxSum += max;
      }
      f64 exp = (f64)curSum / (f64)maxSum;
      check.setEvent(time, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}
