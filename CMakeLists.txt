cmake_minimum_required(VERSION 3.16)
project(stride_benchmark LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags for Release
if(MSVC)
  add_compile_options(/O2 /Oi /Ot /GL /Gy)
  add_link_options(/LTCG)
else()
  add_compile_options(-O3 -march=native -flto)
endif()

# Parser feature flags
option(BENCHMARK_USE_STRIDE "Enable strideparser benchmark" ON)
option(BENCHMARK_USE_SIMDJSON "Enable simdjson benchmark" ON)
option(BENCHMARK_USE_RAPIDJSON "Enable rapidjson benchmark" ON)
option(BENCHMARK_USE_NLOHMANN "Enable nlohmann/json benchmark" ON)
option(BENCHMARK_USE_RAPIDYAML "Enable rapidyaml (ryml) benchmark" ON)
option(BENCHMARK_USE_YAMLCPP "Enable yaml-cpp benchmark" ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ==============================================================================
# 1. Stride Parser
# ==============================================================================
if(BENCHMARK_USE_STRIDE)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/strideparser/CMakeLists.txt")
    add_subdirectory(external/strideparser)
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../boardgame/libgame/external/strideparser/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../boardgame/libgame/external/strideparser" strideparser)
  else()
    FetchContent_Declare(
      strideparser
      GIT_REPOSITORY https://github.com/StrideLang/strideparser.git
      GIT_TAG master
    )
    FetchContent_MakeAvailable(strideparser)
  endif()
endif()

# ==============================================================================
# 2. JSON Parsers
# ==============================================================================
# 2.1 simdjson (State of the art SIMD JSON parser)
if(BENCHMARK_USE_SIMDJSON)
  set(SIMDJSON_DEVELOPER_MODE OFF CACHE BOOL "" FORCE)
  set(SIMDJSON_ENABLE_FALLBACK ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    simdjson
    GIT_REPOSITORY https://github.com/simdjson/simdjson.git
    GIT_TAG v3.10.1
  )
  FetchContent_MakeAvailable(simdjson)
endif()

# 2.2 RapidJSON (Tencent header-only DOM/SAX JSON parser)
if(BENCHMARK_USE_RAPIDJSON)
  set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
  set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG master
  )
  FetchContent_MakeAvailable(rapidjson)
endif()

# 2.3 nlohmann/json (Standard modern C++ JSON reference)
if(BENCHMARK_USE_NLOHMANN)
  set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# ==============================================================================
# 3. YAML Parsers
# ==============================================================================
# 3.1 rapidyaml (ryml: High performance zero-copy YAML parser)
if(BENCHMARK_USE_RAPIDYAML)
  set(RYML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(RYML_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(RYML_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    rapidyaml
    GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
    GIT_TAG v0.5.0
    GIT_SUBMODULES_RECURSE ON
  )
  FetchContent_MakeAvailable(rapidyaml)
endif()

# 3.2 yaml-cpp (Standard C++ YAML reference library)
if(BENCHMARK_USE_YAMLCPP)
  set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    yaml_cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
  )
  FetchContent_MakeAvailable(yaml_cpp)
endif()

# ==============================================================================
# Benchmark Executable Target
# ==============================================================================
add_executable(stride_benchmark src/main.cpp)

target_include_directories(stride_benchmark PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link Stride
if(BENCHMARK_USE_STRIDE AND TARGET strideparser)
  target_link_libraries(stride_benchmark PRIVATE strideparser)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_STRIDE)
endif()

# Link simdjson
if(BENCHMARK_USE_SIMDJSON AND TARGET simdjson)
  target_link_libraries(stride_benchmark PRIVATE simdjson)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_SIMDJSON)
endif()

# Link RapidJSON (header only)
if(BENCHMARK_USE_RAPIDJSON AND rapidjson_POPULATED)
  target_include_directories(stride_benchmark PRIVATE ${rapidjson_SOURCE_DIR}/include)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_RAPIDJSON)
endif()

# Link nlohmann/json
if(BENCHMARK_USE_NLOHMANN AND TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(stride_benchmark PRIVATE nlohmann_json::nlohmann_json)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_NLOHMANN)
endif()

# Link rapidyaml (ryml)
if(BENCHMARK_USE_RAPIDYAML AND TARGET ryml)
  target_link_libraries(stride_benchmark PRIVATE ryml)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_RAPIDYAML)
endif()

# Link yaml-cpp
if(BENCHMARK_USE_YAMLCPP AND TARGET yaml-cpp)
  target_link_libraries(stride_benchmark PRIVATE yaml-cpp)
  target_compile_definitions(stride_benchmark PRIVATE BENCHMARK_HAS_YAMLCPP)
endif()

# Copy data directory next to executable on build
add_custom_command(TARGET stride_benchmark POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/data"
    "$<TARGET_FILE_DIR:stride_benchmark>/data"
  COMMENT "Copying benchmark datasets to output directory"
)
