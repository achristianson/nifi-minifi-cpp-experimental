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

#ifndef LIBMINIFI_INCLUDE_IO_PASSTHROUGHMEMORYMAP_H_
#define LIBMINIFI_INCLUDE_IO_PASSTHROUGHMEMORYMAP_H_
#include "BaseMemoryMap.h"
#include "Serializable.h"
#include <cstdint>
#include <functional>
#include <iostream>

namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace io {

/**
 * PassthroughMemoryMap allows access to an existing underlying memory buffer.
 */
class PassthroughMemoryMap : public BaseMemoryMap {

public:
  PassthroughMemoryMap(void *buf, size_t map_size)
      : buf_(buf), size_(map_size) {}

  virtual ~PassthroughMemoryMap() { unmap(); }

  /**
   * Gets a the address of the mapped data.
   * @return pointer to the mapped data, or nullptr if not mapped
   **/
  virtual void *getData() { return buf_; }

  /**
   * Gets the size of the memory map.
   * @return size of memory map
   */
  virtual size_t getSize() { return size_; }

  /**
   * Explicitly unmap the memory. Memory will otherwise be unmapped at
   * destruction. After this is called, getData will return nullptr.
   */
  virtual void unmap() {
    for (const auto &f : unmap_hooks_) {
      f(buf_, size_);
    }

    buf_ = nullptr;
    size_ = -1;
  }

  /**
   * Registers a callback function to be called when the memory is unmapped.
   */
  void registerUnmapHook(std::function<void(void *, size_t)> f) {
    unmap_hooks_.push_back(f);
  }

protected:
  void *buf_;
  size_t size_;

  std::vector<std::function<void(void *, size_t)>> unmap_hooks_;
};

} /* namespace io */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */
#endif /* LIBMINIFI_INCLUDE_IO_PASSTHROUGHMEMORYMAP_H_ */
