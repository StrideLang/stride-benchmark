#include "benchmark_harness.hpp"
#include "parsers/json_bench.hpp"
#include "parsers/stride_bench.hpp"
#include "parsers/yaml_bench.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct CliArgs {
  size_t iterations{50};
  size_t warmup{5};
  std::string dataset{"all"};
  std::string format{"all"};
  std::string dataDir{"data"};
  bool markdown{false};
  std::string jsonOut;
  bool showHelp{false};
};

static CliArgs parseCli(int argc, char **argv) {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      args.showHelp = true;
    } else if ((arg == "--iterations" || arg == "-i") && i + 1 < argc) {
      args.iterations = std::stoul(argv[++i]);
    } else if ((arg == "--warmup" || arg == "-w") && i + 1 < argc) {
      args.warmup = std::stoul(argv[++i]);
    } else if ((arg == "--dataset" || arg == "-d") && i + 1 < argc) {
      args.dataset = argv[++i];
    } else if ((arg == "--format" || arg == "-f") && i + 1 < argc) {
      args.format = argv[++i];
    } else if (arg == "--data-dir" && i + 1 < argc) {
      args.dataDir = argv[++i];
    } else if (arg == "--markdown" || arg == "-m") {
      args.markdown = true;
    } else if (arg == "--json-output" && i + 1 < argc) {
      args.jsonOut = argv[++i];
    }
  }
  return args;
}

static void printHelp(const char *progName) {
  std::cout << "Usage: " << progName << " [options]\n\n"
            << "Options:\n"
            << "  -i, --iterations <N>    Number of benchmark iterations (default: 50)\n"
            << "  -w, --warmup <N>        Number of warmup iterations (default: 5)\n"
            << "  -d, --dataset <name>    Dataset: config, entities, dataset, nested, or all (default: all)\n"
            << "  -f, --format <fmt>      Format: stride, json, yaml, or all (default: all)\n"
            << "      --data-dir <path>   Directory containing benchmark datasets (default: 'data')\n"
            << "  -m, --markdown          Print results formatted as GitHub Markdown\n"
            << "      --json-output <file> Export benchmark results to a JSON file\n"
            << "  -h, --help              Show this help message\n";
}

static fs::path resolveDataDir(const std::string &hint) {
  if (fs::exists(hint) && fs::is_directory(hint)) {
    return fs::canonical(hint);
  }
  // Check relative to executable directory or parent directories
  std::vector<fs::path> candidates = {
      "data",
      "../data",
      "../../data",
      "../../../data"
  };
  for (const auto &p : candidates) {
    if (fs::exists(p) && fs::is_directory(p)) {
      return fs::canonical(p);
    }
  }
  return fs::path(hint);
}

int main(int argc, char **argv) {
  CliArgs args = parseCli(argc, argv);
  if (args.showHelp) {
    printHelp(argv[0]);
    return 0;
  }

  fs::path dataPath = resolveDataDir(args.dataDir);
  if (!fs::exists(dataPath)) {
    std::cerr << "Error: Benchmark data directory not found at: " << dataPath
              << "\nRun scripts/generate_datasets.py to generate datasets.\n";
    return 1;
  }

  std::cout << "=== Stride vs. JSON vs. YAML Parsing Benchmark ===\n";
  std::cout << "Data directory: " << dataPath.string() << "\n";
  std::cout << "Iterations: " << args.iterations
            << " (warmup: " << args.warmup << ")\n\n";

  benchmark::BenchmarkConfig config;
  config.iterations = args.iterations;
  config.warmupRuns = args.warmup;
  config.markdownOutput = args.markdown;

  benchmark::BenchmarkHarness harness(config);

  std::vector<std::string> datasets;
  if (args.dataset == "all") {
    datasets = {"config", "nested", "entities", "dataset"};
  } else {
    datasets = {args.dataset};
  }

  bool doStride = (args.format == "all" || args.format == "stride");
  bool doJson = (args.format == "all" || args.format == "json");
  bool doYaml = (args.format == "all" || args.format == "yaml");

  for (const auto &ds : datasets) {
    std::cout << ">> Benchmarking dataset: " << ds << "...\n";

    fs::path strideFile = dataPath / (ds + ".stride");
    fs::path jsonFile = dataPath / (ds + ".json");
    fs::path yamlFile = dataPath / (ds + ".yaml");

    // 1. STRIDE
    if (doStride && fs::exists(strideFile)) {
      size_t size = fs::file_size(strideFile);
      std::string pathStr = strideFile.string();
      harness.run("strideparser", "Stride", ds, size, [&]() {
        auto tree = benchmark::StrideBenchmarkParser::parseFile(pathStr);
        if (!tree) {
          throw std::runtime_error("strideparser returned null AST");
        }
      });
    }

    // 2. JSON
    if (doJson && fs::exists(jsonFile)) {
      size_t size = fs::file_size(jsonFile);
      std::string pathStr = jsonFile.string();
      std::string content = benchmark::JsonBenchmarkParser::readFile(pathStr);

#if defined(BENCHMARK_HAS_SIMDJSON)
      // simdjson DOM parser
      simdjson::padded_string padded(content);
      harness.run("simdjson", "JSON", ds, size, [&]() {
        bool ok = benchmark::JsonBenchmarkParser::parseSimdJsonBuffer(padded);
        if (!ok) throw std::runtime_error("simdjson parse failed");
      });
#endif

#if defined(BENCHMARK_HAS_RAPIDJSON)
      // RapidJSON Document parse
      harness.run("rapidjson", "JSON", ds, size, [&]() {
        bool ok = benchmark::JsonBenchmarkParser::parseRapidJson(content);
        if (!ok) throw std::runtime_error("rapidjson parse failed");
      });
#endif

#if defined(BENCHMARK_HAS_NLOHMANN)
      // nlohmann/json parse
      harness.run("nlohmann/json", "JSON", ds, size, [&]() {
        bool ok = benchmark::JsonBenchmarkParser::parseNlohmannJson(content);
        if (!ok) throw std::runtime_error("nlohmann parse failed");
      });
#endif
    }

    // 3. YAML
    if (doYaml && fs::exists(yamlFile)) {
      size_t size = fs::file_size(yamlFile);
      std::string pathStr = yamlFile.string();
      std::string content = benchmark::YamlBenchmarkParser::readFile(pathStr);

#if defined(BENCHMARK_HAS_RAPIDYAML)
      // RapidYAML (ryml)
      harness.run("rapidyaml", "YAML", ds, size, [&]() {
        bool ok = benchmark::YamlBenchmarkParser::parseRapidYaml(content);
        if (!ok) throw std::runtime_error("rapidyaml parse failed");
      });
#endif

#if defined(BENCHMARK_HAS_YAMLCPP)
      // yaml-cpp
      harness.run("yaml-cpp", "YAML", ds, size, [&]() {
        bool ok = benchmark::YamlBenchmarkParser::parseYamlCpp(content);
        if (!ok) throw std::runtime_error("yaml-cpp parse failed");
      });
#endif
    }
  }

  // Display results
  harness.printSummaryTable();

  if (args.markdown) {
    std::cout << harness.toMarkdown() << "\n";
  }

  if (!args.jsonOut.empty()) {
    harness.exportJson(args.jsonOut);
    std::cout << "Exported results to: " << args.jsonOut << "\n";
  }

  return 0;
}
