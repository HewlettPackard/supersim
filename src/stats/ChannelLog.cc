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
#include "stats/ChannelLog.h"

#include <cassert>

ChannelLog::ChannelLog(u32 _numVcs, Json::Value _settings)
    : numVcs_(_numVcs), outFile_(_settings["file"].asString()) {
  assert(!_settings["file"].isNull());

  // set up the stream
  ss_.precision(6);
  ss_.setf(std::ios::fixed, std::ios::floatfield);

  // write the header
  ss_ << "name,";
  for (u32 vc = 0; vc < numVcs_; vc++) {
    ss_ << vc << ',';
  }
  ss_ << "total" << std::endl;

  // write to the outfile and reset the stream
  outFile_.write(ss_.str());
  ss_.str("");
  ss_.clear();
}

ChannelLog::~ChannelLog() {}

void ChannelLog::logChannel(const Channel* _channel) {
  // log the channel utilization to the stream
  ss_ << _channel->fullName() << ',';
  for (u32 vc = 0; vc < numVcs_; vc++) {
    ss_ << _channel->utilization(vc) << ',';
  }
  ss_ << _channel->utilization(U32_MAX) << std::endl;

  // write to the outfile and reset the stream
  outFile_.write(ss_.str());
  ss_.str("");
  ss_.clear();
}
