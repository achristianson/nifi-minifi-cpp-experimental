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

template<class T>
class MemoryMapBMFixture : public benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State &state) {
    testController = std::make_shared<TestController>();
    repo = std::make_shared<T>();
    repo->initialize(std::make_shared<minifi::Configure>());
    char format[] = "/tmp/testRepo.XXXXXX";
    auto dir = std::string(testController->createTempDirectory(format));
    test_file = dir + "/testfile";
    claim = std::make_shared<minifi::ResourceClaim>(test_file, repo);
  }

  void TearDown(const ::benchmark::State &state) {
  }

  void set_test_input(size_t size, char c) {
    test_string = "";
    test_string.resize(size, c);
    auto mm = repo->mmap(claim, test_string.length(), false);
    mm->resize(test_string.length());
    memcpy(mm->getData(), &test_string[0], test_string.length());
  }

  void set_test_expected_output(size_t size, char c) {
    expected_string = "";
    expected_string.resize(size, c);
  }

  void validate_string(const char *read_string) {
    if (strncmp(read_string, expected_string.c_str(), expected_string.length()) != 0) {
      throw std::runtime_error("string read failed");
    }
  }

  void validate_byte(size_t pos, const char b) {
    if (b != expected_string[pos]) {
      throw std::runtime_error("byte read failed");
    }
  }

  /**
   * Get deterministic random points to access. Alternates between positions relative to start & end of file so as to not be sequential.
   * @return set of random points
   */
  std::vector<size_t> random_points() {
    std::vector<size_t> p;

    for (size_t i = 0; i < test_string.length() / 2; i += test_string.length() / 100) {
      p.push_back(i);
      p.push_back(test_string.length() - 1);
    }

    return p;
  }

  std::shared_ptr<TestController> testController;
  std::shared_ptr<T> repo;
  std::shared_ptr<minifi::ResourceClaim> claim;
  std::string test_file;
  std::string test_string;
  std::string expected_string;
};

typedef MemoryMapBMFixture<core::repository::FileSystemRepository> FSMemoryMapBMFixture;
typedef MemoryMapBMFixture<core::repository::VolatileContentRepository> VolatileMemoryMapBMFixture;

template<class T>
void mmap_read(T *fixture, benchmark::State &st, bool read_only) {
  while (st.KeepRunning()) {
    auto mm = fixture->repo->mmap(fixture->claim, fixture->test_string.length(), read_only);
    fixture->validate_string(reinterpret_cast<const char *>(mm->getData()));
  }
}

template<class T>
void mmap_read_random(T *fixture, benchmark::State &st) {
  auto r = fixture->random_points();
  auto mm = fixture->repo->mmap(fixture->claim, fixture->test_string.length(), true);
  while (st.KeepRunning()) {
    auto data = reinterpret_cast<char *>(mm->getData());
    for (size_t p : r) {
      fixture->validate_byte(p, data[p]);
    }
  }
}

template<class T>
void mmap_write_read(T *fixture, benchmark::State &st) {
  while (st.KeepRunning()) {
    auto mm = fixture->repo->mmap(fixture->claim, fixture->test_string.length(), false);
    memcpy(mm->getData(), &(fixture->expected_string[0]), fixture->test_string.length());
    fixture->validate_string(reinterpret_cast<const char *>(mm->getData()));
  }
}

template<class T>
void cb_read(T *fixture, benchmark::State &st) {
  while (st.KeepRunning()) {
    auto rs = fixture->repo->read(fixture->claim);
    std::vector<uint8_t> buf;
    rs->readData(buf, fixture->test_string.length() + 1);
    fixture->validate_string(reinterpret_cast<const char *>(&buf[0]));
  }
}

template<class T>
void cb_read_random(T *fixture, benchmark::State &st) {
  auto r = fixture->random_points();
  auto rs = fixture->repo->read(fixture->claim);
  while (st.KeepRunning()) {
    for (size_t p : r) {
      rs->seek(p);
      char b;
      rs->read(b);
      fixture->validate_byte(p, b);
    }
  }
}

template<class T>
void cb_write_read(T *fixture, benchmark::State &st) {
  while (st.KeepRunning()) {
    {
      auto ws = fixture->repo->write(fixture->claim, false);
      ws->write(reinterpret_cast<uint8_t *>(&(fixture->expected_string[0])), fixture->test_string.length());
    }

    auto rs = fixture->repo->read(fixture->claim);
    std::vector<uint8_t> buf;
    rs->readData(buf, fixture->test_string.length() + 1);
    fixture->validate_string(reinterpret_cast<const char *>(&buf[0]));
  }
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_RO_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, true);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_Read_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'x');
  cb_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_WriteRead_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'y');
  mmap_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_WriteRead_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'y');
  cb_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_RO_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, true);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_Read_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'x');
  cb_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_WriteRead_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'y');
  mmap_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_WriteRead_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'y');
  cb_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_Read_RO_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  mmap_read<FSMemoryMapBMFixture>(this, st, true);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_Read_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  cb_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_WriteRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'y');
  mmap_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_WriteRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'y');
  cb_write_read<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, MemoryMap_FileSystemRepository_RandomRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  mmap_read_random<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(FSMemoryMapBMFixture, Callback_FileSystemRepository_RandomRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  cb_read_random<FSMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_Read_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'x');
  mmap_read<VolatileMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_Read_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'x');
  cb_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_WriteRead_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'y');
  mmap_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_WriteRead_Tiny)(benchmark::State &st) {
  set_test_input(10, 'x');
  set_test_expected_output(10, 'y');
  cb_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_Read_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'x');
  mmap_read<VolatileMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_Read_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'x');
  cb_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_WriteRead_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'y');
  mmap_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_WriteRead_Small)(benchmark::State &st) {
  set_test_input(131072, 'x');
  set_test_expected_output(131072, 'y');
  cb_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_Read_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  mmap_read<VolatileMemoryMapBMFixture>(this, st, false);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_Read_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  cb_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_WriteRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'y');
  mmap_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_WriteRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'y');
  cb_write_read<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, MemoryMap_VolatileRepository_RandomRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  mmap_read_random<VolatileMemoryMapBMFixture>(this, st);
}

BENCHMARK_F(VolatileMemoryMapBMFixture, Callback_VolatileRepository_RandomRead_Large)(benchmark::State &st) {
  set_test_input(33554432, 'x');
  set_test_expected_output(33554432, 'x');
  cb_read_random<VolatileMemoryMapBMFixture>(this, st);
}

//more tests:
// duplicate all tests for volatile, db repos
BENCHMARK_MAIN();
