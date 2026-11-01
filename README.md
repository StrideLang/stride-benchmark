# Stride vs. JSON & YAML Parsing Benchmark

A standalone CMake benchmarking suite comparing the parsing performance and throughput of **Stride** against industry-standard **JSON** and **YAML** parsers.

---

## 1. Overview & Motivation

Stride was originally designed as a real-time reactive DSP and visual programming language. With its clean declarative syntax and strong compile-time schema validation capabilities, Stride is equally suitable as a **pure data storage and configuration language** (similar to JSON, YAML, or TOML).

This benchmark evaluates how Stride's native parser performs relative to state-of-the-art parsers across equivalent workloads:

| Format | Parser | Type | Description |
|---|---|---|---|
| **Stride** | [`strideparser`](https://github.com/StrideLang/strideparser) | Flex / Bison LALR(1) | Native AST parser generating full declarative AST nodes. |
| **JSON** | [`simdjson`](https://github.com/simdjson/simdjson) | SIMD vectorized | State-of-the-art parser using AVX2/NEON vector instructions (often > 2 GB/s). |
| **JSON** | [`RapidJSON`](https://github.com/Tencent/rapidjson) | SAX / DOM | Highly optimized header-only C++ JSON parser by Tencent. |
| **JSON** | [`nlohmann/json`](https://github.com/nlohmann/json) | DOM | Industry-standard modern C++ JSON reference library. |
| **YAML** | [`rapidyaml` (`ryml`)](https://github.com/biojppm/rapidyaml) | In-situ / Arena | High-performance zero-copy YAML parser (orders of magnitude faster than yaml-cpp). |
| **YAML** | [`yaml-cpp`](https://github.com/jbeder/yaml-cpp) | DOM | Standard C++ YAML parser and reference library. |

---

## 2. Benchmark Datasets

The suite benchmarks equivalent datasets across all three formats:

1. **`config` (Small, ~300 B)**:
   - Typical application / game configuration with scalars, strings, numbers, booleans, and arrays.
2. **`nested` (Hierarchy Stress Test, ~3–6 KB)**:
   - Deeply nested recursive structure with 20 hierarchy levels to stress-test parser grammar recursion and call stacks.
3. **`entities` (Medium, ~25–35 KB)**:
   - 100 game units with unique IDs, names, roles, health/mana stats, floating-point coordinates, and inventory lists.
4. **`dataset` (Large, ~350–520 KB)**:
   - 1,500 structured game entities and table entries representing realistic data serialization payloads.

---

## 3. Building

The project is completely self-contained using CMake and `FetchContent`. It automatically downloads and configures all required dependencies.

```powershell
# Clone with submodules
git clone --recurse-submodules <repo_url>
cd stride-benchmark

# Configure with CMake (Release mode strongly recommended for benchmarks)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

### Build Options

Each parser can be individually enabled or disabled via CMake options:

```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release `
  -DBENCHMARK_USE_STRIDE=ON `
  -DBENCHMARK_USE_SIMDJSON=ON `
  -DBENCHMARK_USE_RAPIDJSON=ON `
  -DBENCHMARK_USE_NLOHMANN=ON `
  -DBENCHMARK_USE_RAPIDYAML=ON `
  -DBENCHMARK_USE_YAMLCPP=ON
```

---

## 4. Running the Benchmark

```powershell
# Run all benchmarks with default 50 iterations
./build/Release/stride_benchmark.exe

# Custom iteration count and warmup
./build/Release/stride_benchmark.exe --iterations 100 --warmup 10

# Filter by dataset (config, nested, entities, dataset, all)
./build/Release/stride_benchmark.exe --dataset entities

# Filter by format (stride, json, yaml, all)
./build/Release/stride_benchmark.exe --format stride

# Output results as GitHub Markdown table
./build/Release/stride_benchmark.exe --markdown

# Export raw timing metrics to JSON
./build/Release/stride_benchmark.exe --json-output results.json
```

---

## 5. Generating Datasets

To regenerate or scale the benchmark datasets to custom entity counts:

```powershell
python scripts/generate_datasets.py
```

---

## 6. Metrics & Methodology

- **Warmup Runs**: Runs $N$ unmeasured warmup iterations per parser to prime CPU caches and OS disk buffers.
- **Timing Accuracy**: Uses `std::chrono::high_resolution_clock` around the core parse routines.
- **Statistical Measures**: Reports `Mean`, `Median`, `Min`, `Max`, and `Standard Deviation` across all iterations.
- **Throughput**: Calculated as:
  $$\text{Throughput (MB/s)} = \frac{\text{File Size (MB)}}{\text{Mean Parse Time (seconds)}}$$
