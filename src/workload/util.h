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
#ifndef WORKLOAD_UTIL_H_
#define WORKLOAD_UTIL_H_

#include <prim/prim.h>

/*
 * This generates a 64-bit transaction ID.
 */
u64 transactionId(u32 _appId, u32 _termId, u32 _msgId);

/*
 * This extracts the 8-bit application ID from the transaction ID.
 */
u32 appId(u64 _transId);

/*
 * This computes how many cycles it would take to send a packet with the
 *  specified number of flits. Probabilistic injection is used when the number
 *  of cycles isn't a deterministic value.
 */
u64 cyclesToSend(f64 _injectionRate, u32 _numFlits);

#endif  // WORKLOAD_UTIL_H_
