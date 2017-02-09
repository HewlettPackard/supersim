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
#include "traffic/ReadWriteMessageSizeDistribution.h"

#include <gtest/gtest.h>
#include <json/json.h>

#include "test/TestSetup_TEST.h"
#include "traffic/MessageSizeDistributionFactory.h"
#include "types/Message.h"
#include "types/Packet.h"

TEST(ReadWriteMessageSizeDistribution, simple) {
  TestSetup ts(123, 123, 123);

  // size must all be different for this test fixture to work correctly
  const u32 RQ = 2;
  const u32 RS = 10;
  const u32 WQ = 11;
  const u32 WS = 1;
  const f64 RP = 0.75;


  Json::Value settings;
  settings["type"] = "read_write";
  settings["read_request_size"] = RQ;
  settings["read_response_size"] = RS;
  settings["write_request_size"] = WQ;
  settings["write_response_size"] = WS;
  settings["read_probability"] = RP;

  MessageSizeDistribution* msd =
      MessageSizeDistributionFactory::createMessageSizeDistribution(
          "msd", nullptr, settings);

  std::unordered_map<u32, u32> sizes;
  const u32 ROUNDS = 10000000;
  Message* fakeRequest = new Message(1, nullptr);
  Packet* fakeRQ = new Packet(0, RQ, fakeRequest);
  Packet* fakeWQ = new Packet(0, WQ, fakeRequest);
  for (u32 round = 0; round < ROUNDS; round++) {
    // request
    u32 req = msd->nextMessageSize();
    ASSERT_TRUE(req == RQ || req == WQ);
    sizes[req]++;

    // make a fake message to pass in as the request message
    Packet* fakePkt = (req == RQ) ? fakeRQ : fakeWQ;
    fakeRequest->setPacket(0, fakePkt);

    // response
    u32 res = msd->nextMessageSize(fakeRequest);
    ASSERT_TRUE(res == RS || res == WS);
    sizes[res]++;
  }
  delete fakeRQ;
  delete fakeWQ;
  fakeRequest->setPacket(0, nullptr);
  delete fakeRequest;

  // check sizes
  for (auto& sizePair : sizes) {
    u32 size = sizePair.first;
    u32 count = sizePair.second;

    f64 act = (f64)count / ROUNDS;
    f64 exp;
    switch (size) {
      case RQ:
      case RS:
        exp = RP;
        break;

      case WQ:
      case WS:
        exp = (1.0 - RP);
        break;

      default:
        assert(false);  // can this happen?
        break;
    }

    ASSERT_NEAR(act, exp, 0.001);
  }


  delete msd;
}
