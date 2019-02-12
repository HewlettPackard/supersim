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

#include "congestion/CongestionSensor.h"
#include "congestion/Congestion_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(BufferOccupancy, normVc) {
  TestSetup test(1, 1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
      std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value sensorSettings;
  sensorSettings["latency"] = latency;
  sensorSettings["granularity"] = granularity;
  sensorSettings["mode"] = "normalized_vc";
  sensorSettings["minimum"] = 0;
  sensorSettings["offset"] = 0;
  BufferOccupancy sensor("CongestionSensor", &router, &router,
                         sensorSettings);
  sensor.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      sensor.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &sensor);
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

  time = 10000;
  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      // check credits
      u32 max = port * 10 + vc + 2;
      f64 exp = 2.0 / (f64)max;
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  time = 100000;
  for (u32 incr = 1; incr <= 2; incr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::INCR);

        // advance time
        time++;
      }
    }
  }

  gSim->initialize();
  gSim->simulate();
}

TEST(BufferOccupancy, absVc) {
  TestSetup test(1, 1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
      std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value sensorSettings;
  sensorSettings["latency"] = latency;
  sensorSettings["granularity"] = granularity;
  sensorSettings["mode"] = "absolute_vc";
  sensorSettings["minimum"] = 0;
  sensorSettings["offset"] = 0;
  BufferOccupancy sensor("CongestionSensor", &router, &router,
                         sensorSettings);
  sensor.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      sensor.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &sensor);
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

  time = 10000;
  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      // check credits
      f64 exp = 2.0;
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  time = 100000;
  for (u32 incr = 1; incr <= 2; incr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::INCR);

        // advance time
        time++;
      }
    }
  }

  gSim->initialize();
  gSim->simulate();
}

TEST(BufferOccupancy, normPort) {
  TestSetup test(1, 1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
      std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value sensorSettings;
  sensorSettings["latency"] = latency;
  sensorSettings["granularity"] = granularity;
  sensorSettings["mode"] = "normalized_port";
  sensorSettings["minimum"] = 0;
  sensorSettings["offset"] = 0;
  BufferOccupancy sensor("CongestionSensor", &router, &router,
                         sensorSettings);
  sensor.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      sensor.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &sensor);
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

  time = 10000;
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

  time = 100000;
  for (u32 incr = 1; incr <= 2; incr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::INCR);

        // advance time
        time++;
      }
    }
  }

  gSim->initialize();
  gSim->simulate();
}

TEST(BufferOccupancy, absPort) {
  TestSetup test(1, 1, 1, 1234);

  const bool debug = false;
  const u32 numPorts = 5;
  const u32 numVcs = 4;
  const u32 latency = 8;
  const u32 granularity = 0;

  Json::Value routerSettings;
  CongestionTestRouter router(
      "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
      std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
  router.setDebug(debug);

  Json::Value sensorSettings;
  sensorSettings["latency"] = latency;
  sensorSettings["granularity"] = granularity;
  sensorSettings["mode"] = "absolute_port";
  sensorSettings["minimum"] = 0;
  sensorSettings["offset"] = 0;
  BufferOccupancy sensor("CongestionSensor", &router, &router,
                         sensorSettings);
  sensor.setDebug(debug);

  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      u32 max = port * 10 + vc + 2;
      sensor.initCredits(router.vcIndex(port, vc), max);
    }
  }

  CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
  crediter.setDebug(debug);

  StatusCheck check("StatusCheck", nullptr, &sensor);
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

  time = 10000;
  for (u32 port = 0; port < numPorts; port++) {
    for (u32 vc = 0; vc < numVcs; vc++) {
      // check credits
      u32 curSum = 0;
      for (u32 vc2 = 0; vc2 < numVcs; vc2++) {
        curSum += 2;
      }
      f64 exp = (f64)curSum;
      check.setEvent(time, 0, 0, 0, port, vc, exp);
    }
  }

  time = 10000;
  for (u32 incr = 1; incr <= 2; incr++) {
    for (u32 port = 0; port < numPorts; port++) {
      for (u32 vc = 0; vc < numVcs; vc++) {
        // modify credits
        crediter.setEvent(port, vc, time, 1, CreditHandler::Type::INCR);

        // advance time
        time++;
      }
    }
  }

  gSim->initialize();
  gSim->simulate();
}

TEST(BufferOccupancy, phantomNormVc) {
  const bool debug = false;
  const u32 numPorts = 1;
  const u32 numVcs = 1;
  const u32 sensorLatency = 1;
  const u32 bufferDepth = 200;

  const u32 granularity = 0;

  for (f64 valueCoeff = 0.5; valueCoeff < 3.0; valueCoeff += 0.34) {
    for (f64 lengthCoeff = 0.5; lengthCoeff < 3.0; lengthCoeff += 0.34) {
      for (u32 channelLatency = 3; channelLatency <= 40; channelLatency += 3) {
        TestSetup test(1, 1, 1, 1234);

        Json::Value routerSettings;
        CongestionTestRouter router(
            "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
            std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
        router.setDebug(debug);

        Json::Value channelSettings;
        channelSettings["latency"] = channelLatency;
        Channel channel("Channel", nullptr, 8, channelSettings);
        router.setOutputChannel(0, &channel);

        Json::Value sensorSettings;
        sensorSettings["latency"] = sensorLatency;
        sensorSettings["granularity"] = granularity;
        sensorSettings["phantom"] = true;
        sensorSettings["value_coeff"] = valueCoeff;
        sensorSettings["length_coeff"] = lengthCoeff;
        sensorSettings["mode"] = "normalized_vc";
        sensorSettings["minimum"] = 0;
        sensorSettings["offset"] = 0;
        BufferOccupancy sensor("CongestionSensor", &router, &router,
                               sensorSettings);
        sensor.setDebug(debug);

        sensor.initCredits(router.vcIndex(0, 0), bufferDepth);

        CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
        crediter.setDebug(debug);

        StatusCheck check("StatusCheck", nullptr, &sensor);
        check.setDebug(debug);

        // send flits (decrement credits)
        u64 time = 1000;
        for (u32 flit = 0; flit < bufferDepth; flit++) {
          crediter.setEvent(0, 0, time, 1, CreditHandler::Type::DECR);
          time++;
        }

        // create events for sensor checking
        for (u32 ch = 0; ch < 100; ch++) {
          f64 inWindow = (f64)(u32)(channelLatency * lengthCoeff) -
                         std::min((u32)(channelLatency * lengthCoeff), ch);
          f64 exp = ((f64)bufferDepth - inWindow * valueCoeff) /
                    (f64)bufferDepth;
          exp = std::min(1.0, std::max(0.0, exp));
          check.setEvent(time, 0, 0, 0, 0, 0, exp);
          time++;
        }

        for (u32 flit = 0; flit < bufferDepth; flit++) {
          crediter.setEvent(0, 0, time, 1, CreditHandler::Type::INCR);
        }

        gSim->initialize();
        gSim->simulate();
      }
    }
  }
}

TEST(BufferOccupancy, phantomAbsVc) {
  const bool debug = false;
  const u32 numPorts = 1;
  const u32 numVcs = 1;
  const u32 sensorLatency = 1;
  const u32 bufferDepth = 200;

  const u32 granularity = 0;

  for (f64 valueCoeff = 0.5; valueCoeff < 3.0; valueCoeff += 0.34) {
    for (f64 lengthCoeff = 0.5; lengthCoeff < 3.0; lengthCoeff += 0.34) {
      for (u32 channelLatency = 3; channelLatency <= 40; channelLatency += 3) {
        TestSetup test(1, 1, 1, 1234);

        Json::Value routerSettings;
        CongestionTestRouter router(
            "Router", nullptr, nullptr, 0, std::vector<u32>(), numPorts, numVcs,
            std::vector<std::tuple<u32, u32> >(), nullptr, routerSettings);
        router.setDebug(debug);

        Json::Value channelSettings;
        channelSettings["latency"] = channelLatency;
        Channel channel("Channel", nullptr, 8, channelSettings);
        router.setOutputChannel(0, &channel);

        Json::Value sensorSettings;
        sensorSettings["latency"] = sensorLatency;
        sensorSettings["granularity"] = granularity;
        sensorSettings["phantom"] = true;
        sensorSettings["value_coeff"] = valueCoeff;
        sensorSettings["length_coeff"] = lengthCoeff;
        sensorSettings["mode"] = "absolute_vc";
        sensorSettings["minimum"] = 0;
        sensorSettings["offset"] = 0;
        BufferOccupancy sensor("CongestionSensor", &router, &router,
                               sensorSettings);
        sensor.setDebug(debug);

        sensor.initCredits(router.vcIndex(0, 0), bufferDepth);

        CreditHandler crediter("CreditHandler", nullptr, &sensor, &router);
        crediter.setDebug(debug);

        StatusCheck check("StatusCheck", nullptr, &sensor);
        check.setDebug(debug);

        // send flits (decrement credits)
        u64 time = 1000;
        for (u32 flit = 0; flit < bufferDepth; flit++) {
          crediter.setEvent(0, 0, time, 1, CreditHandler::Type::DECR);
          time++;
        }

        // create events for sensor checking
        for (u32 ch = 0; ch < 100; ch++) {
          f64 inWindow = (f64)(u32)(channelLatency * lengthCoeff) -
                         std::min((u32)(channelLatency * lengthCoeff), ch);
          f64 exp = (f64)bufferDepth - inWindow * valueCoeff;
          exp = std::max(0.0, exp);
          check.setEvent(time, 0, 0, 0, 0, 0, exp);
          time++;
        }

        for (u32 flit = 0; flit < bufferDepth; flit++) {
          crediter.setEvent(0, 0, time, 1, CreditHandler::Type::INCR);
        }

        gSim->initialize();
        gSim->simulate();
      }
    }
  }
}
