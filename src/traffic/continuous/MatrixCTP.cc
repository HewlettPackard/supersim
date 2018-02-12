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
#include "traffic/continuous/MatrixCTP.h"

#include <factory/ObjectFactory.h>
#include <fio/InFile.h>
#include <mut/mut.h>
#include <strop/strop.h>

#include <cassert>

MatrixCTP::MatrixCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  // make space to hold a probability distribution
  std::vector<f64> probabilityDistribution(numTerminals_, F64_POS_INF);

  // parse the probability distribution from the settings file
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
            probabilityDistribution.at(idx) = std::stod(strs.at(idx));
          }
        }
        lineNum++;
      }
    }
  }
  if (lineNum != numTerminals_) {
    fprintf(stderr, "expected %u lines, processed %u lines\n",
            numTerminals_, lineNum);
    assert(false);
  }

  // create the cumulative distribution
  mut::generateCumulativeDistribution(
      probabilityDistribution, &cumulativeDistribution_);
}

MatrixCTP::~MatrixCTP() {}

u32 MatrixCTP::nextDestination() {
  f64 rnd = gSim->rnd.nextF64();
  return mut::searchCumulativeDistribution(cumulativeDistribution_, rnd);
}

registerWithObjectFactory("matrix", ContinuousTrafficPattern,
                          MatrixCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
