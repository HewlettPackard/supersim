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

#include <gtest/gtest.h>
#include <json/json.h>
// #include <mut/mut.h>
#include <prim/prim.h>
// #include <strop/strop.h>

#include <cstdio>

#include <tuple>
#include <vector>

#include "test/TestSetup_TEST.h"

const u32 SIZE4 = 4;
const f64 DIST4[] = {
  0.25, 0.25, 0.25, 0.25,
  0.50, 0.20, 0.20, 0.10,
  0.05, 0.30, 0.40, 0.25,
  0.01, 0.96, 0.02, 0.01};

const u32 SIZE8 = 8;
const f64 DIST8[] = {
  0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125,
  0.250, 0.250, 0.100, 0.100, 0.100, 0.100, 0.050, 0.050,
  0.025, 0.025, 0.150, 0.150, 0.200, 0.200, 0.125, 0.125,
  0.005, 0.005, 0.480, 0.480, 0.010, 0.010, 0.005, 0.005,
  0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125,
  0.250, 0.250, 0.100, 0.100, 0.100, 0.100, 0.050, 0.050,
  0.025, 0.025, 0.150, 0.150, 0.200, 0.200, 0.125, 0.125,
  0.005, 0.005, 0.480, 0.480, 0.010, 0.010, 0.005, 0.005};

const u32 SIZE16 = 16;
const f64 DIST16[] = {
  0.8500, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100,
  0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100,
  0.0200, 0.7000, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200,
  0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200,
  0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500,
  0.0500, 0.0500, 0.0500, 0.2500, 0.0500, 0.0500, 0.0500,
  0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625,
  0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625,
  0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.1250,
  0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250,
  0.1250, 0.1250, 0.1250, 0.1250, 0.0000, 0.0000, 0.0000, 0.0000, 0.1250,
  0.1250, 0.1250, 0.1250, 0.0000, 0.0000, 0.0000, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.1250, 0.1250, 0.1250, 0.1250, 0.0000,
  0.0000, 0.0000, 0.0000, 0.1250, 0.1250, 0.1250, 0.1250,
  0.0000, 0.1000, 0.0000, 0.1000, 0.0000, 0.1000, 0.1000, 0.1000, 0.1000,
  0.1000, 0.0000, 0.1000, 0.0000, 0.1000, 0.0000, 0.1000,
  0.1000, 0.1000, 0.1000, 0.1000, 0.1000, 0.0000, 0.0000, 0.0000, 0.0000,
  0.0000, 0.1000, 0.0000, 0.1000, 0.1000, 0.1000, 0.1000,
  0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100,
  0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.0100, 0.8500,
  0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.0200,
  0.0200, 0.0200, 0.0200, 0.0200, 0.0200, 0.7000, 0.0200,
  0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500, 0.0500,
  0.0500, 0.0500, 0.0500, 0.2500, 0.0500, 0.0500, 0.0500,
  0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625,
  0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625, 0.0625,
  0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.1250, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 0.0000,
  0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};

std::vector<std::tuple<u32, const f64*> > allTests() {
  std::vector<std::tuple<u32, const f64*> > tests;
  tests.push_back(std::make_tuple(SIZE4, DIST4));
  tests.push_back(std::make_tuple(SIZE8, DIST8));
  tests.push_back(std::make_tuple(SIZE16, DIST16));
  return tests;
}


const char* TMPFILE = "temptrafficmatrix.csv";

u32 makeIdx(u32 _row, u32 _col, u32 _size) {
  return _size * _row + _col;
}

std::string makeCSV(const f64* _dist, u32 _size) {
  std::stringstream ss;
  for (u32 row = 0; row < _size; row++) {
    for (u32 col = 0; col < _size; col++) {
      ss << _dist[makeIdx(row, col, _size)] << ',';
    }
    ss << '\n';
  }
  return ss.str();
}

void writeCSV(const char* _csv, const char* _file) {
  FILE* fout = fopen(_file, "w");
  assert(fout);
  fwrite(_csv, sizeof(char), strlen(_csv), fout);
  fclose(fout);
}

void cleanCSV(const char* _file) {
  std::remove(_file);
}

TEST(MatrixTrafficPattern, full) {
  const bool DEBUG = false;
  for (const std::tuple<u32, const f64*>& test : allTests()) {
    u32 testSize = std::get<0>(test);
    const f64* testDist = std::get<1>(test);

    // set up
    TestSetup testSetup(1234, 1234, 456789);
    writeCSV(makeCSV(testDist, testSize).c_str(), TMPFILE);
    Json::Value settings;
    settings["file"] = TMPFILE;
    std::vector<TrafficPattern*> tps(testSize, nullptr);
    for (u32 tp = 0; tp < testSize; tp++) {
      tps.at(tp) = new MatrixTrafficPattern(
          "TP_" + std::to_string(tp), nullptr, testSize, tp, settings);
    }

    // testing
    f64* counts = new f64[testSize * testSize]();
    u32* srcCounts = new u32[testSize]();
    for (u32 r = 0; r < 20000000; r++) {
      u32 src = gSim->rnd.nextU64(0, testSize - 1);
      u32 dst = tps.at(src)->nextDestination();
      srcCounts[src]++;
      counts[makeIdx(src, dst, testSize)]++;
    }

    // turn counts into dist
    for (u32 src = 0; src < testSize; src++) {
      for (u32 dst = 0; dst < testSize; dst++) {
        counts[makeIdx(src, dst, testSize)] /= (f64)srcCounts[src];
      }
    }

    // print results
    if (DEBUG) {
      printf("Expected:\n%s\n", makeCSV(testDist, testSize).c_str());
      printf("Actual:\n%s\n", makeCSV(counts, testSize).c_str());
    }

    // verification
    f64 worst = 0.0;
    for (u32 src = 0; src < testSize; src++) {
      for (u32 dst = 0; dst < testSize; dst++) {
        f64 act = counts[makeIdx(src, dst, testSize)];
        f64 exp = testDist[makeIdx(src, dst, testSize)];
        ASSERT_NEAR(act, exp, 0.001);
        if (fabs(act - exp) > worst) {
          worst = fabs(act - exp);
        }
      }
    }
    if (DEBUG) {
      printf("worst variance=%f\n", worst);
    }

    // clean up
    delete[] counts;
    delete[] srcCounts;
    for (u32 tp = 0; tp < testSize; tp++) {
      delete tps.at(tp);
    }
    cleanCSV(TMPFILE);
  }
}
