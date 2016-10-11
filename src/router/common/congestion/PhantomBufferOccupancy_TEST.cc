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
#include "router/common/congestion/PhantomBufferOccupancy.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include "network/Channel.h"
#include "router/common/congestion/CongestionStatus.h"
#include "router/common/congestion/CongestionStatus_TEST.h"
#include "test/TestSetup_TEST.h"

TEST(PhantomBufferOccupancy, statusCheck) {
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
        routerSettings["num_ports"] = numPorts;
        routerSettings["num_vcs"] = numVcs;
        CongestionTestRouter router("Router", nullptr, std::vector<u32>({}),
                                    nullptr, routerSettings);
        router.setDebug(debug);

        Json::Value channelSettings;
        channelSettings["latency"] = channelLatency;
        Channel channel("Channel", nullptr, channelSettings);
        router.setOutputChannel(0, &channel);

        Json::Value statusSettings;
        statusSettings["latency"] = statusLatency;
        statusSettings["granularity"] = granularity;
        statusSettings["value_coeff"] = valueCoeff;
        statusSettings["length_coeff"] = lengthCoeff;
        PhantomBufferOccupancy status("CongestionStatus", &router, &router,
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
          crediter.setEvent(0, 0, time, 1, CongestionStatus::DECR);
          time++;
        }

        // create events for status checking
        for (u32 ch = 0; ch < 100; ch++) {
          f64 inWindow = (f64)(u32)(channelLatency * lengthCoeff) -
              std::min((u32)(channelLatency * lengthCoeff), ch);
          f64 exp = ((f64)bufferDepth - inWindow * valueCoeff) /
              (f64)bufferDepth;
          exp = std::min(1.0, std::max(0.0, exp));
          check.setEvent(time, 0, 0, 0, exp);
          time++;
        }

        gSim->simulate();
      }
    }
  }
}
