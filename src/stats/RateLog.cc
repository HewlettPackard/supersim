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
#include "stats/RateLog.h"

#include <cassert>

RateLog::RateLog(Json::Value _settings)
    : outFile_(nullptr) {
  if (!_settings["file"].isNull()) {
    // create file
    outFile_ = new fio::OutFile(_settings["file"].asString());

    // write header
    outFile_->write("id,name,injection,delivered,ejection\n");
    ss_.precision(6);
    ss_.setf(std::ios::fixed, std::ios::floatfield);
  }
}

RateLog::~RateLog() {
  if (outFile_) {
    delete outFile_;
  }
}

void RateLog::logRates(u32 _terminalId, const std::string& _terminalName,
                       f64 _injectionRate, f64 _deliveredRate,
                       f64 _ejectionRate) {
  if (outFile_) {
    ss_ << _terminalId << ',' << _terminalName << ',' << _injectionRate << ','
        << _deliveredRate << ',' << _ejectionRate << '\n';
    outFile_->write(ss_.str());
    ss_.str("");
    ss_.clear();
  }
}
