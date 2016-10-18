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
#include "traffic/MatrixTrafficPattern.h"

#include <fio/InFile.h>
#include <strop/strop.h>

#include <cassert>

MatrixTrafficPattern::MatrixTrafficPattern(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : TrafficPattern(_name, _parent, _numTerminals, _self) {

  // make space to hold a distribution
  dist_.resize(numTerminals_, F64_POS_INF);

  // parse the distribution from the settings file
  assert(_settings.isMember("file") && _settings["file"].isString());
  fio::InFile inf(_settings["file"].asString());
  std::string line;
  u32 lineNum = 0;
  fio::InFile::Status sts = fio::InFile::Status::OK;
  for (lineNum = 0; sts == fio::InFile::Status::OK;) {
    sts = inf.getLine(&line);
    assert(sts != fio::InFile::Status::ERROR);
    if (sts == fio::InFile::Status::OK) {
      if (line.size() > 0) {
        if (lineNum == self_) {
          std::vector<std::string> strs = strop::split(line, ',');
          assert(strs.size() == numTerminals_);
          for (u32 idx = 0; idx < strs.size(); idx++) {
            dist_.at(idx) = std::stod(strs.at(idx));
          }
        }
        lineNum++;
      }
    }
  }
  assert(lineNum == numTerminals_);

  // verify the current distribution sums to 1.0
  // reformat distribution for a binary search
  f64 sum = 0.0;
  for (f64& f : dist_) {
    f64 t = f;
    f = sum;
    sum += t;
  }
  // if (fabs(1.0 - sum) >= 0.000001) {
  //   printf("lineNum=%u tol=%f\n", lineNum, fabs(1.0 - sum));
  // }
  assert(abs(1.0 - sum) < 0.000001);
}

MatrixTrafficPattern::~MatrixTrafficPattern() {}

u32 MatrixTrafficPattern::nextDestination() {
  f64 rnd = gSim->rnd.nextF64();

  u32 bot = 0;
  u32 top = numTerminals_;

  while (true) {
    assert(top > bot);
    u32 span = top - bot;
    u32 mid = (span / 2) + bot;
    if (span == 1) {
      // done! return the index
      return mid;
    } else if (dist_[mid] < rnd) {
      // raise the bottom
      bot = mid;
    } else {
      // lower the top
      top = mid;
    }
  }
}
