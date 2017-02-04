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
#include "workload/util.h"

#include <gtest/gtest.h>
#include <prim/prim.h>

#include "event/Simulator.h"
#include "test/TestSetup_TEST.h"

TEST(WorkloadUtil, transactionId) {
  u32 appId, termId, msgId;

  appId = 0;
  termId = 0;
  msgId = 0;
  ASSERT_EQ(0lu, transactionId(appId, termId, msgId));

  appId = 255;
  termId = 0;
  msgId = 0;
  ASSERT_EQ(0xFF00000000000000lu, transactionId(appId, termId, msgId));

  appId = 0;
  termId = 16777215;
  msgId = 0;
  ASSERT_EQ(0x00FFFFFF00000000lu, transactionId(appId, termId, msgId));

  appId = 0;
  termId = 0;
  msgId = 4294967295;
  ASSERT_EQ(0x00000000FFFFFFFFlu, transactionId(appId, termId, msgId));

  appId = 255;
  termId = 16777215;
  msgId = 4294967295;
  ASSERT_EQ(0xFFFFFFFFFFFFFFFFlu, transactionId(appId, termId, msgId));

  appId = 1;
  termId = 1;
  msgId = 1;
  ASSERT_EQ(0x0100000100000001lu, transactionId(appId, termId, msgId));

  appId = 128;
  termId = 8388608;
  msgId = 2147483648;
  ASSERT_EQ(0x8080000080000000lu, transactionId(appId, termId, msgId));
}

TEST(WorkloadUtil, appId) {
  u64 transId;

  transId = 0x0100000000000000lu;
  ASSERT_EQ(1u, appId(transId));

  transId = 0x0200000000000000lu;
  ASSERT_EQ(2u, appId(transId));

  transId = 0x0A00000000000000lu;
  ASSERT_EQ(10u, appId(transId));

  transId = 0x8000000000000000lu;
  ASSERT_EQ(128u, appId(transId));

  transId = 0xFF00000000000000lu;
  ASSERT_EQ(255u, appId(transId));

  transId = 0x8800000000000000lu;
  ASSERT_EQ(136u, appId(transId));
}

TEST(WorkloadUtil, cyclesToSend_multiple) {
  TestSetup ts(123, 123, 123);

  const u32 kRounds = 1000000;
  for (u32 r = 0; r < kRounds; r++) {
    ASSERT_EQ(cyclesToSend(1.0000, 16), 16u);
    ASSERT_EQ(cyclesToSend(0.5000, 16), 32u);
    ASSERT_EQ(cyclesToSend(0.2500, 16), 64u);
    ASSERT_EQ(cyclesToSend(0.1250, 16), 128u);
    ASSERT_EQ(cyclesToSend(0.0625, 16), 256u);
  }
}

TEST(WorkloadUtil, cyclesToSend_probabilistic) {
  TestSetup ts(123, 123, 123);

  const u32 kTests = 500;
  const u32 kRounds = 1000000;

  for (u32 t = 0; t < kTests; t++) {
    f64 rate = std::max(0.001, gSim->rnd.nextF64());
    u32 flits = gSim->rnd.nextU64(1, 50);
    f64 sum = 0;
    for (u32 r = 0; r < kRounds; r++) {
      sum += (f64)cyclesToSend(rate, flits);
    }
    f64 act = sum / kRounds;
    f64 exp = (f64)flits * (1 / rate);
    ASSERT_NEAR(act, exp, 0.002);
  }
}
