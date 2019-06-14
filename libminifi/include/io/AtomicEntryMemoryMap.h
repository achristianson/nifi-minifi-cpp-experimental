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
#ifndef LIBMINIFI_INCLUDE_IO_ATOMICENTRYMEMORYMAP_H_
#define LIBMINIFI_INCLUDE_IO_ATOMICENTRYMEMORYMAP_H_

#include <mutex>
#include <cstring>
#include "BaseMemoryMap.h"
#include "core/repository/AtomicRepoEntries.h"
#include "Exception.h"
#include "core/logging/LoggerConfiguration.h"
namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace io {

template<typename T>
class AtomicEntryMemoryMap : public BaseMemoryMap {
 public:
  AtomicEntryMemoryMap(const T key, core::repository::AtomicEntry<T> *entry, size_t map_size)
      : key_(key),
        entry_(entry),
        logger_(logging::LoggerFactory<AtomicEntryMemoryMap()>::getLogger()) {
    if (entry_->getValue(key, &value_)) {
      value_->resize(map_size);
      entry_->decrementOwnership();
      invalid_stream_ = false;
    } else {
      invalid_stream_ = true;
    }
  }

  virtual ~AtomicEntryMemoryMap() {
    entry_->decrementOwnership();
  }

  virtual void unmap() {

  }

  virtual size_t getSize() {
    if(invalid_stream_) {
        return -1;
    }

    return value_->getBufferSize();
  }

  virtual void *getData() {
      if (invalid_stream_) {
          return nullptr;
      }

      return (void *)(value_->getBuffer());
  }

 protected:
  T key_;
  core::repository::AtomicEntry<T> *entry_;
  core::repository::RepoValue<T> *value_;
  std::atomic<bool> invalid_stream_;

  // Logger
  std::shared_ptr<logging::Logger> logger_;

};

} /* namespace io */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */

#endif /* LIBMINIFI_INCLUDE_IO_ATOMICENTRYMEMORYMAP_H_ */