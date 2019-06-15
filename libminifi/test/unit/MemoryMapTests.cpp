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

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "../TestBase.h"
#include "ResourceClaim.h"
#include "core/Core.h"
#include "properties/Configure.h"

TEST_CASE("MemoryMap Test Test", "[MemoryMapTest1]") { REQUIRE(true); }

TEST_CASE("MemoryMap FileSystemRepository Read", "[MemoryMapTest2]") {
  auto fsr = std::make_shared<core::repository::FileSystemRepository>();
  TestController testController;
  char format[] = "/tmp/testRepo.XXXXXX";
  auto dir = std::string(testController.createTempDirectory(format));
  auto test_file = dir + "/testfile";

  {
    std::ofstream os(test_file);
    os << "hello";
  }

  auto claim = std::make_shared<minifi::ResourceClaim>(test_file, fsr);
  auto mm = fsr->mmap(claim, 1024, false);
  std::string read_string(reinterpret_cast<const char *>(mm->getData()));
  REQUIRE(read_string == "hello");
}

TEST_CASE("MemoryMap FileSystemRepository Write", "[MemoryMapTest3]") {
  auto fsr = std::make_shared<core::repository::FileSystemRepository>();
  TestController testController;
  char format[] = "/tmp/testRepo.XXXXXX";
  auto dir = std::string(testController.createTempDirectory(format));
  auto test_file = dir + "/testfile";
  auto claim = std::make_shared<minifi::ResourceClaim>(test_file, fsr);
  auto mm = fsr->mmap(claim, 1024, false);
  std::string write_test_string("write test");
  std::memcpy(reinterpret_cast<char *>(mm->getData()), write_test_string.c_str(),
              write_test_string.length());
  std::string read_string(reinterpret_cast<const char *>(mm->getData()));
  std::stringstream iss;
  {
    std::ifstream is(test_file);
    iss << is.rdbuf();
  }
  REQUIRE(read_string == "write test");
}

TEST_CASE("MemoryMap VolatileContentRepository Write/Read", "[MemoryMapTest3]") {
  auto vr = std::make_shared<core::repository::VolatileContentRepository>();
  auto c = std::make_shared<minifi::Configure>();
  vr->initialize(c);
  TestController testController;
  char format[] = "/tmp/testRepo.XXXXXX";
  auto dir = std::string(testController.createTempDirectory(format));
  auto test_file = dir + "/testfile";
  std::string write_test_string("test read val");

  auto claim = std::make_shared<minifi::ResourceClaim>(test_file, vr);

  {
    auto mm = vr->mmap(claim, 1024, false);
    std::memcpy(reinterpret_cast<char *>(mm->getData()), write_test_string.c_str(),
                write_test_string.length());
  }

  {
    auto mm = vr->mmap(claim, 1024, false);
    std::string read_string(reinterpret_cast<const char *>(mm->getData()));
    REQUIRE(read_string == "test read val");
  }
}
