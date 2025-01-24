cmake_policy(SET CMP0048 NEW)
project(libcharta LANGUAGES CXX C VERSION 0.1.0)
cmake_minimum_required (VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

option(BUILD_COVERAGE "Build coverage" OFF)
option(BUILD_TESTING "Build tests" ON)
option(BUILD_USE_SANITIZER "Use address sanitizer to find any memory leaks" OFF)
option(BUILD_APPS "Build applications" ON)

if(BUILD_USE_SANITIZER)
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

if(BUILD_COVERAGE)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif()

add_subdirectory(contrib)
add_subdirectory(include)
add_subdirectory(src)
if(BUILD_APPS)
    add_subdirectory(apps)
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

include(CPack)