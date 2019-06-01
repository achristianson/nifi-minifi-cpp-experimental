# Licensed to the Apache Software Foundation (ASF) under one
#
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

cmake_minimum_required(VERSION 3.0)
project(nifi-minifi-cpp-ll)

if($ENV{DEBUG})
  message(WARNING, "DEBUG MODE IS ON")
  set(OPT_FLAGS "-g3")
else()
  set(OPT_FLAGS "-O2")
endif()

set(CMAKE_C_FLAGS "-Wall -Werror ${OPT_FLAGS} -Wfatal-errors")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -fPIC")
endif()

set(USE_STANDALONE_ASIO ON CACHE INTERNAL "")
add_subdirectory("../thirdparty/Simple-Web-Server-v3.0.0-rc3" build)
include_directories("../thirdparty/Simple-Web-Server-v3.0.0-rc3")

add_executable(minifi-ll-editor editor.cpp)
set_property(TARGET minifi-ll-editor PROPERTY CXX_STANDARD 17)

target_link_libraries(minifi-ll-editor simple-web-server)

file(COPY static DESTINATION .)
