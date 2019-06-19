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
#include <cstdint>
#include <functional>
#include <iostream>
#include "BaseMemoryMap.h"
#include "Serializable.h"

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
  PassthroughMemoryMap(std::function<void *()> buf_fn, std::function<size_t()> map_size_fn, std::function<void *(size_t)> resize_fn)
      : buf_fn_(buf_fn), size_fn_(map_size_fn), resize_fn_(resize_fn) {}

  virtual ~PassthroughMemoryMap() { unmap(); }

  /**
   * Gets a the address of the mapped data.
   * @return pointer to the mapped data, or nullptr if not mapped
   **/
  virtual void *getData() { return buf_fn_(); }

  /**
   * Gets the size of the memory map.
   * @return size of memory map
   */
  virtual size_t getSize() { return size_fn_(); }

  /**
   * Resize the underlying file.
   * @return pointer to the remapped data
   */
  virtual void *resize(size_t new_size) { return resize_fn_(new_size); }

  /**
   * Explicitly unmap the memory. Memory will otherwise be unmapped at
   * destruction. After this is called, getData will return nullptr.
   */
  virtual void unmap() {
    auto s = size_fn_();
    auto buf = buf_fn_();
    for (const auto &f : unmap_hooks_) {
      f(buf, s);
    }
  }

  /**
   * Registers a callback function to be called when the memory is unmapped.
   */
  void registerUnmapHook(std::function<void(void *, size_t)> f) { unmap_hooks_.push_back(f); }

 protected:
  std::function<void *()> buf_fn_;
  std::function<size_t()> size_fn_;
  std::function<void *(size_t)> resize_fn_;

  std::vector<std::function<void(void *, size_t)>> unmap_hooks_;
};

} /* namespace io */
} /* namespace minifi */
} /* namespace nifi */
} /* namespace apache */
} /* namespace org */
#endif /* LIBMINIFI_INCLUDE_IO_PASSTHROUGHMEMORYMAP_H_ */
