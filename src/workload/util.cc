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

u64 transactionId(u32 _appId, u32 _termId, u32 _msgId) {
  return ((u64)_appId << 56) | ((u64)_termId << 32) | ((u64)_msgId);
}

u32 appId(u64 _transId) {
  return (u32)(_transId >> 56);
}
