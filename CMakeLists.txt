cmake_minimum_required(VERSION 3.20)
project(pratik_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(PRATIK_ENABLE_CUDA "Build the CUDA backend (needs nvcc, sm_80+)" OFF)

add_executable(pratik_mvp src/main.cpp src/cpu_backend.cpp)
target_include_directories(pratik_mvp PRIVATE include)
target_compile_options(pratik_mvp PRIVATE -O2 -Wall -Wextra)
find_package(Threads REQUIRED)
target_link_libraries(pratik_mvp PRIVATE Threads::Threads)

if(PRATIK_ENABLE_CUDA)
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 20)
  set(CMAKE_CUDA_ARCHITECTURES 80)
  target_sources(pratik_mvp PRIVATE cuda/cuda_backend.cu)
  target_compile_definitions(pratik_mvp PRIVATE PRATIK_ENABLE_CUDA)
  set_source_files_properties(cuda/cuda_backend.cu PROPERTIES
    COMPILE_OPTIONS "--expt-relaxed-constexpr")
  message(STATUS "PRATIK: CUDA backend enabled (sm_80)")
else()
  message(STATUS "PRATIK: CPU-only build (configure -DPRATIK_ENABLE_CUDA=ON for GPU)")
endif()
