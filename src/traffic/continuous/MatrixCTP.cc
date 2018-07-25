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

namespace {

// this holds cummulative distributions
std::unordered_map<std::string, std::vector<std::vector<f64> > > cdistMap;
bool cleared = false;

// this retrieves cummulative distributions, loads them only when needed
const std::vector<f64>& retrieveCummulativeDistribution(
    const std::string& _filename, u32 _terminal, u32 _numTerminals) {
  cleared = false;

  // determine if file has already been processed
  if (cdistMap.count(_filename) == 0) {
    // create a new entry
    cdistMap[_filename];
    std::vector<std::vector<f64> >& cdists = cdistMap.at(_filename);

    // parse the file
    fio::InFile inf(_filename);
    std::string line;
    u32 lineNum = 0;
    fio::InFile::Status sts = fio::InFile::Status::OK;
    for (lineNum = 0; sts == fio::InFile::Status::OK;) {
      sts = inf.getLine(&line);
      assert(sts != fio::InFile::Status::ERROR);
      if (sts == fio::InFile::Status::OK) {
        if (line.size() > 0) {
          // make space to hold a probability distribution
          std::vector<f64> pdist(_numTerminals, F64_POS_INF);

          // insert prob dist data
          std::vector<std::string> strs = strop::split(line, ',');
          assert(strs.size() == _numTerminals);
          for (u32 idx = 0; idx < strs.size(); idx++) {
            pdist.at(idx) = std::stod(strs.at(idx));
          }

          // create the cumulative distribution
          cdists.push_back({});
          std::vector<f64>& cdist = cdists.at(lineNum);
          mut::generateCumulativeDistribution(pdist, &cdist);

          // advance line counter
          lineNum++;
        }
      }
    }
    if (lineNum != _numTerminals) {
      fprintf(stderr, "expected %u lines, processed %u lines\n",
              _numTerminals, lineNum);
      assert(false);
    }
  }

  // retrieve the cummulative distribution
  return cdistMap.at(_filename).at(_terminal);
}

// this clears the cummulative distributions
void clearCummulativeDistributions() {
  if (!cleared) {
    cleared = true;
    cdistMap.clear();
  }
}

}  // namespace

MatrixCTP::MatrixCTP(
    const std::string& _name, const Component* _parent, u32 _numTerminals,
    u32 _self, Json::Value _settings)
    : ContinuousTrafficPattern(_name, _parent, _numTerminals, _self,
                               _settings) {
  assert(_settings.isMember("file") && _settings["file"].isString());
  cumulativeDistribution_ = retrieveCummulativeDistribution(
      _settings["file"].asString(), self_, numTerminals_);
}

MatrixCTP::~MatrixCTP() {}

u32 MatrixCTP::nextDestination() {
  clearCummulativeDistributions();
  f64 rnd = gSim->rnd.nextF64();
  return mut::searchCumulativeDistribution(cumulativeDistribution_, rnd);
}

registerWithObjectFactory("matrix", ContinuousTrafficPattern,
                          MatrixCTP, CONTINUOUSTRAFFICPATTERN_ARGS);
