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
#include "congestion/BufferOccupancy.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include "congestion/CongestionStatus.h"
#include "congestion/CongestionStatus_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(BufferOccupancy, statusCheck_NormVcMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, {}, numPorts, numVcs, nullptr,
      routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "normalized_vc";
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
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::DECR);

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
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}

TEST(BufferOccupancy, statusCheck_AbsVcMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, {}, numPorts, numVcs, nullptr,
      routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "absolute_vc";
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
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::DECR);

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
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}

TEST(BufferOccupancy, statusCheck_NormPortMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, {}, numPorts, numVcs, nullptr,
      routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "normalized_port";
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
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::DECR);

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
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}

TEST(BufferOccupancy, statusCheck_AbsPortMode) {
  TestSetup test(1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, {}, numPorts, numVcs, nullptr,
      routerSettings);
  router.setDebug(debug);

  Json::Value statusSettings;
  statusSettings["latency"] = latency;
  statusSettings["granularity"] = granularity;
  statusSettings["mode"] = "absolute_port";
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
  for (u32 decr = 1; decr <= 2; decr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::DECR);

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
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  gSim->simulate();
}

TEST(BufferOccupancy, phantomStatusCheck) {
  const bool debug = false;
  const u32 numPorts = 1;
  const u32 numVcs = 1;
  const u32 statusLatency = 1;
  const u32 bufferDepth = 200;

  const u32 granularity = 0;

  for (f64 valueCoeff = 0.5; valueCoeff < 3.0; valueCoeff += 0.34) {
    for (f64 lengthCoeff = 0.5; lengthCoeff < 3.0; lengthCoeff += 0.34) {
      for (u32 channelLatency = 3; channelLatency <= 40; channelLatency += 3) {
        TestSetup test(1, 1, 1234);

        Json::Value routerSettings;
        CongestionTestRouter router(
            "Router", nullptr, nullptr, 0, {}, numPorts, numVcs, nullptr,
            routerSettings);
        router.setDebug(debug);

        Json::Value channelSettings;
        channelSettings["latency"] = channelLatency;
        Channel channel("Channel", nullptr, 8, channelSettings);
        router.setOutputChannel(0, &channel);

        Json::Value statusSettings;
        statusSettings["latency"] = statusLatency;
        statusSettings["granularity"] = granularity;
        statusSettings["phantom"] = true;
        statusSettings["value_coeff"] = valueCoeff;
        statusSettings["length_coeff"] = lengthCoeff;
        statusSettings["mode"] = "absolute_vc";
        BufferOccupancy status("CongestionStatus", &router, &router,
                               statusSettings);
        status.setDebug(debug);

        status.initCredits(router.vcIndex(0, 0), bufferDepth);

        CreditHandler crediter("CreditHandler", nullptr, &status, &router);
        crediter.setDebug(debug);

        StatusCheck check("StatusCheck", nullptr, &status);
        check.setDebug(debug);

        // send flits (decrement credits)
        u64 time = 1000;
        for (u32 flit = 0; flit < bufferDepth; flit++) {
          crediter.setEvent(0, 0, time, 1, CreditHandler::Type::DECR);
          time++;
        }

        // create events for status checking
        for (u32 ch = 0; ch < 100; ch++) {
          f64 inWindow = (f64)(u32)(channelLatency * lengthCoeff) -
              std::min((u32)(channelLatency * lengthCoeff), ch);
          f64 exp = ((f64)bufferDepth - inWindow * valueCoeff) /
              (f64)bufferDepth;
          exp = std::min(1.0, std::max(0.0, exp));
          check.setEvent(time, 0, 0, 0, 0, 0, exp);
          time++;
        }

        gSim->simulate();
      }
    }
  }
}
