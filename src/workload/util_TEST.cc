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

TEST(Application, transactionId) {
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

TEST(Application, appId) {
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
