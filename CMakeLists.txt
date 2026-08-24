cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
set(VCPKG_INSTALL_OPTIONS "--no-print-usage")
list(APPEND VCPKG_INSTALL_OPTIONS "--x-buildtrees-root=${CMAKE_CURRENT_LIST_DIR}/.cache/buildtrees")
list(APPEND VCPKG_INSTALL_OPTIONS "--x-packages-root=${CMAKE_CURRENT_LIST_DIR}/.cache/packages")
if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT EXISTS ${CMAKE_TOOLCHAIN_FILE})
  unset(CMAKE_TOOLCHAIN_FILE CACHE)
endif()
if(DEFINED ENV{X_VCPKG_REGISTRIES_CACHE})
  file(MAKE_DIRECTORY $ENV{X_VCPKG_REGISTRIES_CACHE})
endif()
project(multithreading DESCRIPTION "Multithreading" VERSION 0.2.0 LANGUAGES CXX)

set(THIRD_PARTY_LIBRARIES)
if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake$")
  find_package(TBB CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES TBB::tbb TBB::tbbmalloc)

  find_package(Boost REQUIRED COMPONENTS asio lockfree)
  list(APPEND THIRD_PARTY_LIBRARIES Boost::asio Boost::lockfree)

  find_package(concurrentqueue CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES concurrentqueue::concurrentqueue)

  find_package(spdlog CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES spdlog::spdlog_header_only)

  find_package(MPMCQueue CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES MPMCQueue::MPMCQueue)

  find_package(SPSCQueue CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES SPSCQueue::SPSCQueue)

  find_package(quill CONFIG REQUIRED)
  list(APPEND THIRD_PARTY_LIBRARIES quill::quill)
endif()

add_library(logger OBJECT src/logger.hpp src/logger.cpp)
target_link_libraries(logger PUBLIC ${THIRD_PARTY_LIBRARIES})
target_include_directories(logger PUBLIC src)

if(WIN32)
  target_compile_definitions(logger PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE logger)

file(GLOB LOGGER_HEADERS src/logger.hpp solutions/*.hpp)
if(WIN32 AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/solutions/windows/slist.hpp)
  list(APPEND LOGGER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/solutions/windows/slist.hpp)
endif()

add_subdirectory(src/tests)
add_subdirectory(src/benchmarks)

if(DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()
if(DEFINED CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE})
endif()
if(DEFINED CMAKE_VERBOSE_MAKEFILE)
  set(CMAKE_VERBOSE_MAKEFILE ${CMAKE_BUILD_TYPE})
endif()
