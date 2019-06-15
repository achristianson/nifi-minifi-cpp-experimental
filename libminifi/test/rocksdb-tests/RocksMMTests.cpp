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
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include "../TestBase.h"
#include "../unit/ProvenanceTestHelper.h"
#include "DatabaseContentRepository.h"
#include "FlowFileRecord.h"
#include "FlowFileRepository.h"
#include "core/Core.h"
#include "core/RepositoryFactory.h"
#include "properties/Configure.h"
#include "provenance/Provenance.h"

TEST_CASE("DatabaseContentRepository Write/Read", "[RocksMMTest1]") {
  TestController testController;
  char format[] = "/tmp/testRepo.XXXXXX";
  auto dir = std::string(testController.createTempDirectory(format));
  auto test_file = dir + "/testfile";

  auto configuration = std::make_shared<org::apache::nifi::minifi::Configure>();
  configuration->set(minifi::Configure::nifi_dbcontent_repository_directory_default, dir);

  auto dbr = std::make_shared<core::repository::DatabaseContentRepository>();
  REQUIRE(true == dbr->initialize(configuration));

  std::string write_test_string("test read val");

  auto claim = std::make_shared<minifi::ResourceClaim>(test_file, dbr);

  {
    auto mm = dbr->mmap(claim, 1024, true);
    REQUIRE(mm != nullptr);
    std::memcpy(reinterpret_cast<char *>(mm->getData()), write_test_string.c_str(),
                write_test_string.length());
  }

  {
    auto mm = dbr->mmap(claim, 1024, true);
    REQUIRE(mm != nullptr);
    std::string read_string(reinterpret_cast<const char *>(mm->getData()));
    REQUIRE(read_string == "test read val");
  }
}
