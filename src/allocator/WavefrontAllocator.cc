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
#include "allocator/WavefrontAllocator.h"

#include <factory/ObjectFactory.h>

#include <cassert>

#include "event/Simulator.h"

WavefrontAllocator::WavefrontAllocator(
    const std::string& _name, const Component* _parent,
    u32 _numClients, u32 _numResources, Json::Value _settings)
    : Allocator(_name, _parent, _numClients, _numResources, _settings) {
  // pointer vectors
  requests_.resize(numClients_ * numResources_, nullptr);
  grants_.resize(numClients_ * numResources_, nullptr);

  // shape
  if (numClients_ > numResources_) {
    rows_ = numClients_;
    cols_ = numResources_;
  } else {
    rows_ = numResources_;
    cols_ = numClients_;
  }

  // priority scheme
  std::string scheme = _settings["scheme"].asString();
  if (scheme == "sequential") {
    scheme_ = WavefrontAllocator::PriorityScheme::kSequential;
  } else if (scheme == "random") {
    scheme_ = WavefrontAllocator::PriorityScheme::kRandom;
  } else {
    fprintf(stderr, "invalid wavefront priority scheme: %s\n", scheme.c_str());
    assert(false);
  }

  // grant vectors
  colGrants_.resize(cols_, false);
  rowGrants_.resize(rows_, false);

  // init priority state
  startingLine_ = gSim->rnd.nextU64(0, rows_ - 1);
}

WavefrontAllocator::~WavefrontAllocator() {}

void WavefrontAllocator::setRequest(u32 _client, u32 _resource,
                                    bool* _request) {
  u32 row, col;
  toRowCol(_client, _resource, &row, &col);
  requests_[toIndex(row, col)] = _request;
}

void WavefrontAllocator::setMetadata(u32 _client, u32 _resource,
                                     u64* _metadata) {}

void WavefrontAllocator::setGrant(u32 _client, u32 _resource, bool* _grant) {
  u32 row, col;
  toRowCol(_client, _resource, &row, &col);
  grants_[toIndex(row, col)] = _grant;
}

void WavefrontAllocator::allocate() {
  // reset the grant vectors
  std::fill(colGrants_.begin(), colGrants_.end(), false);
  std::fill(rowGrants_.begin(), rowGrants_.end(), false);

  // perform wavefront allocation
  for (u32 rOffset = 0; rOffset < rows_; rOffset++) {
    u32 line = (startingLine_ + rOffset) % rows_;
    for (u32 col = 0; col < cols_; col++) {
      // check if the col has already been given a grant
      if (colGrants_.at(col)) {
        continue;
      }

      // derive the resource
      u32 row = toRow(line, col);

      // check if the row has already been given a grant
      if (rowGrants_.at(row)) {
        continue;
      }

      // convert from resource/client to index
      u32 index = toIndex(row, col);

      // allocate if requested
      if (*(requests_.at(index))) {
        *(grants_.at(index)) = true;
        colGrants_.at(col) = true;
        rowGrants_.at(row) = true;
      }
    }
  }

  // update the row for the allocation
  switch (scheme_) {
    case WavefrontAllocator::PriorityScheme::kSequential:
      startingLine_ = (startingLine_ + 1) % rows_;
      break;
    case WavefrontAllocator::PriorityScheme::kRandom:
      startingLine_ = gSim->rnd.nextU64(0, rows_ - 1);
      break;
    default:
      assert(false);
  }
}

void WavefrontAllocator::toRowCol(u32 _client, u32 _resource,
                                  u32* _row, u32* _col) const {
  if (numClients_ > numResources_) {
    *_row = _client;
    *_col = _resource;
  } else {
    *_row = _resource;
    *_col = _client;
  }
}

u32 WavefrontAllocator::toIndex(u32 _row, u32 _col) const {
  u32 line = (_row + _col) % rows_;
  u32 base = line * cols_;
  u32 index = base + _col;
  assert(index < cols_ * rows_);  // REMOVE ME
  return index;
}

u32 WavefrontAllocator::toRow(u32 _line, u32 _col) const {
  u32 row = (_col > _line) ?
            (_line + rows_ - _col) :
            (_line - _col);
  assert(row < rows_);  // REMOVE ME
  return row;
}

registerWithObjectFactory("wavefront", Allocator, WavefrontAllocator,
                          ALLOCATOR_ARGS);
