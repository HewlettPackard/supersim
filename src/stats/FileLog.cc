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
#include "stats/FileLog.h"

#include <prim/prim.h>

#include <cassert>
#include <cstdio>

FileLog::FileLog(Json::Value _settings) {
  std::string filename = _settings["file"].asString();
  u64 namelen = filename.size();
  assert(namelen > 0);
  compress_ = filename.substr(namelen-3) == ".gz";

  bool fail;
  if (compress_) {
    gzFile_ = gzopen(filename.c_str(), "wb");
    fail = (gzFile_ == nullptr);
  } else {
    regFile_ = fopen(filename.c_str(), "wb");
    fail = (regFile_ == nullptr);
  }

  if (fail) {
    fprintf(stderr, "ERROR: couldn't open file '%s'\n", filename.c_str());
    exit(-1);
  }
}

FileLog::~FileLog() {
  if (compress_) {
    gzclose(gzFile_);
  } else {
    fclose(regFile_);
  }
}

void FileLog::write(const std::string& _text) {
  const void* cstr = reinterpret_cast<const void*>(_text.c_str());
  size_t len = _text.size();

  if (compress_) {
    assert(gzwrite(gzFile_, cstr, len) == (s64)len);
  } else {
    assert(fwrite(cstr, sizeof(char), len, regFile_) == len);
  }
}
