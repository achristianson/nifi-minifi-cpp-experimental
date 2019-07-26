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

#include "io/FileMemoryMap.h"

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <mio/mmap.hpp>

namespace org {
namespace apache {
namespace nifi {
namespace minifi {
namespace io {

FileMemoryMap::FileMemoryMap(const std::string &path, size_t map_size, bool read_only)
    : file_data_(nullptr), path_(path), length_(map_size), read_only_(read_only), logger_(logging::LoggerFactory<FileMemoryMap>::getLogger()) {
  map(path, map_size, read_only);
}

void FileMemoryMap::map(const std::string &path, size_t map_size, bool read_only) {
  {
    // ensure file is at least as big as requested map size
    std::fstream f(path, std::fstream::in | std::fstream::out | std::fstream::trunc | std::fstream::ate | std::fstream::binary);
    std::fstream::pos_type file_size = f.tellg();

    if (file_size < 0 || (size_t)file_size < map_size) {
      if (!read_only) {
        logger_->log_info("Resizing file '%s' to '%d' bytes", path, map_size);
        f.seekp(map_size - file_size, std::ios::end);
        f << '\0';
      } else {
        throw std::runtime_error("File is smaller than map size and read-only mode is set: " + path);
      }
    }
  }

  // memory map the file
  std::error_code error;

  if (read_only) {
    ro_mmap_ = mio::make_mmap_source(path, error);
  } else {
    rw_mmap_ = mio::make_mmap_sink(path, error);
  }
  
  if (error) {
    throw std::runtime_error("Failed to memory map file '" + path + "' due to: " + error.message());
  }
}

void FileMemoryMap::unmap() {
  if (read_only_) {
    ro_mmap_.unmap();
  } else {
    std::error_code error;
    rw_mmap_.sync(error);
  
    if (error) {
      throw std::runtime_error("Failed to unmap memory-mapped file due to: " + error.message());
    }

    rw_mmap_.unmap();
  }
}

void *FileMemoryMap::getData() {
  if (read_only_) {
    return &ro_mmap_[0];
  } else {
    return &rw_mmap_[0];
  }
}

size_t FileMemoryMap::getSize() {
  return length_;
}

void *FileMemoryMap::resize(size_t new_size) {
  if (read_only_) {
    throw std::runtime_error("Cannot resize read-only mmap");
  }

  unmap();
  map(path_, new_size, false);
  length_ = new_size;

  return &rw_mmap_[0];
}

}  // namespace io
}  // namespace minifi
}  // namespace nifi
}  // namespace apache
}  // namespace org
