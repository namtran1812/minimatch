cmake_minimum_required(VERSION 3.20)
project(MiniMatch VERSION 1.0.0 LANGUAGES CXX)

find_package(SQLite3 REQUIRED)

if(TARGET SQLite3::SQLite3)
  set(MINIMATCH_SQLITE_TARGET SQLite3::SQLite3)
elseif(TARGET SQLite::SQLite3)
  set(MINIMATCH_SQLITE_TARGET SQLite::SQLite3)
else()
  message(
    FATAL_ERROR
    "SQLite3 was found, but no supported imported target exists."
  )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINIMATCH_BUILD_TESTS "Build tests" ON)
option(MINIMATCH_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

set(
  MINIMATCH_BOOST_ROOT
  ""
  CACHE PATH
  "Root directory of the Boost installation"
)

if(NOT MINIMATCH_BOOST_ROOT)
  if(APPLE AND EXISTS "/opt/homebrew/opt/boost/include/boost/asio.hpp")
    set(
      MINIMATCH_BOOST_ROOT
      "/opt/homebrew/opt/boost"
    )
  elseif(UNIX AND EXISTS "/usr/include/boost/asio.hpp")
    set(
      MINIMATCH_BOOST_ROOT
      "/usr"
    )
  else()
    message(
      FATAL_ERROR
      "Boost headers were not found. Set MINIMATCH_BOOST_ROOT "
      "to the Boost installation prefix."
    )
  endif()
endif()

message(
  STATUS
  "Using Boost root: ${MINIMATCH_BOOST_ROOT}"
)

find_path(
  MINIMATCH_BOOST_INCLUDE_DIR
  NAMES boost/asio.hpp
  HINTS "${MINIMATCH_BOOST_ROOT}/include"
  NO_DEFAULT_PATH
  REQUIRED
)


add_library(minimatch
    src/order_book.cpp
    src/exchange.cpp
    src/event_log.cpp
    src/protocol.cpp
    src/risk.cpp
    src/quant.cpp
    src/execution.cpp
    src/queue_model.cpp
    src/pipeline.cpp
    src/router.cpp
    src/execution_engine.cpp
    src/execution_store.cpp
)
target_include_directories(
  minimatch
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${MINIMATCH_BOOST_INCLUDE_DIR}"
)

target_compile_definitions(
  minimatch
  PUBLIC
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
)

target_link_libraries(
  minimatch
  PUBLIC
    ${MINIMATCH_SQLITE_TARGET}
)
target_compile_options(minimatch PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(MINIMATCH_ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(minimatch PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(minimatch PUBLIC -fsanitize=address,undefined)
endif()

foreach(app server client replay loadgen latency snapshot backtest market_maker options_lab execution_sim research_lab queue_sim web arena)
  add_executable(minimatch_${app} apps/${app}.cpp)
  target_link_libraries(minimatch_${app} PRIVATE minimatch)
endforeach()

if(MINIMATCH_BUILD_TESTS)
  include(FetchContent)
  cmake_policy(SET CMP0135 NEW)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  add_executable(minimatch_tests tests/order_book_test.cpp tests/advanced_test.cpp tests/system_test.cpp tests/quant_test.cpp tests/pipeline_test.cpp tests/router_test.cpp tests/execution_engine_test.cpp tests/execution_store_test.cpp)
  

target_link_libraries(
  minimatch_tests
  PRIVATE
    minimatch
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(minimatch_tests)
endif()
