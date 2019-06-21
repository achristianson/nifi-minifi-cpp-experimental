/**
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <benchmark/benchmark.h>
#include "../test/TestBase.h"
#include "ResourceClaim.h"
#include "core/Core.h"
#include "properties/Configure.h"

class FSMemoryMapFixture : public benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State &state) {
    testController = std::make_shared<TestController>();
    fsr = std::make_shared<core::repository::FileSystemRepository>();
    char format[] = "/tmp/testRepo.XXXXXX";
    auto dir = std::string(testController->createTempDirectory(format));
    test_file = dir + "/testfile";
    test_string = "hello";
    std::ofstream os(test_file);
    os << test_string;
    claim = std::make_shared<minifi::ResourceClaim>(test_file, fsr);
  }

  void TearDown(const ::benchmark::State &state) {
  }

  void validate_string(const char *read_string) {
    if (strcmp(read_string, test_string.c_str()) != 0) {
      throw std::runtime_error("string read failed");
    }
  }

  std::shared_ptr<TestController> testController;
  std::shared_ptr<core::repository::FileSystemRepository> fsr;
  std::shared_ptr<minifi::ResourceClaim> claim;
  std::string test_file;
  std::string test_string;
};

BENCHMARK_F(FSMemoryMapFixture, MemoryMap_FileSystemRepository_Read)(benchmark::State &st) {
  while (st.KeepRunning()) {
    auto mm = fsr->mmap(claim, test_string.length() + 1, false);
    validate_string(reinterpret_cast<const char *>(mm->getData()));
  }
}

BENCHMARK_F(FSMemoryMapFixture, MemoryMap_FileSystemRepository_Read_RO)(benchmark::State &st) {
  while (st.KeepRunning()) {
    auto mm = fsr->mmap(claim, test_string.length() + 1, true);
    validate_string(reinterpret_cast<const char *>(mm->getData()));
  }
}

BENCHMARK_F(FSMemoryMapFixture, Callback_FileSystemRepository_Read)(benchmark::State &st) {
  while (st.KeepRunning()) {
    auto rs = fsr->read(claim);
    std::vector<uint8_t> buf;
    rs->readData(buf, test_string.length() + 1);
    validate_string(reinterpret_cast<const char *>(&buf[0]));
  }
}

static void BM_StringCreation(benchmark::State &state) {
  for (auto _ : state)
    std::string empty_string;
}
// Register the function as a benchmark
BENCHMARK(BM_StringCreation);

// Define another benchmark
static void BM_StringCopy(benchmark::State &state) {
  std::string x = "hello";
  for (auto _ : state)
    std::string copy(x);
}
BENCHMARK(BM_StringCopy);

BENCHMARK_MAIN();
