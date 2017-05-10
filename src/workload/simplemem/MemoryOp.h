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
#ifndef WORKLOAD_SIMPLEMEM_MEMORYOP_H_
#define WORKLOAD_SIMPLEMEM_MEMORYOP_H_

#include <prim/prim.h>

#include "workload/simplemem/MemoryOp.h"

namespace SimpleMem {

class MemoryOp {
 public:
  enum class eOp {kReadReq, kReadResp, kWriteReq, kWriteResp};

  MemoryOp(eOp _op, u32 _address);
  MemoryOp(eOp _op, u32 _address, u32 _blockSize);
  ~MemoryOp();

  eOp op() const;
  u32 address() const;
  u32 blockSize() const;
  u8* block() const;

 private:
  eOp op_;
  u32 address_;
  u32 blockSize_;
  u8* block_;
};

}  // namespace SimpleMem

#endif  // WORKLOAD_SIMPLEMEM_MEMORYOP_H_
