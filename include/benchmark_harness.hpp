#ifndef BENCHMARK_HARNESS_HPP
#define BENCHMARK_HARNESS_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace benchmark {

struct BenchmarkConfig {
  size_t iterations{100};
  size_t warmupRuns{10};
  bool verbose{false};
  bool markdownOutput{false};
  std::string filter;
};

struct BenchmarkResult {
  std::string parserName;
  std::string format;
  std::string datasetName;
  size_t fileSizeBytes{0};
  size_t iterations{0};
  double meanUs{0.0};
  double medianUs{0.0};
  double minUs{0.0};
  double maxUs{0.0};
  double stddevUs{0.0};
  double throughputMBs{0.0};
  bool success{true};
  std::string errorMessage;
};

class BenchmarkHarness {
public:
  explicit BenchmarkHarness(BenchmarkConfig config = {}) : m_config(config) {}

  template <typename Func>
  BenchmarkResult run(const std::string &parserName,
                      const std::string &format,
                      const std::string &datasetName,
                      size_t fileSizeBytes,
                      Func &&parseFunc) {
    BenchmarkResult result;
    result.parserName = parserName;
    result.format = format;
    result.datasetName = datasetName;
    result.fileSizeBytes = fileSizeBytes;
    result.iterations = m_config.iterations;

    // Warmup runs
    for (size_t i = 0; i < m_config.warmupRuns; ++i) {
      try {
        parseFunc();
      } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = e.what();
        m_results.push_back(result);
        return result;
      } catch (...) {
        result.success = false;
        result.errorMessage = "Unknown exception during warmup";
        m_results.push_back(result);
        return result;
      }
    }

    // Measurement runs
    std::vector<double> timingsUs;
    timingsUs.reserve(m_config.iterations);

    for (size_t i = 0; i < m_config.iterations; ++i) {
      auto start = std::chrono::high_resolution_clock::now();
      try {
        parseFunc();
      } catch (const std::exception &e) {
        result.success = false;
        result.errorMessage = e.what();
        m_results.push_back(result);
        return result;
      } catch (...) {
        result.success = false;
        result.errorMessage = "Unknown exception during run";
        m_results.push_back(result);
        return result;
      }
      auto end = std::chrono::high_resolution_clock::now();
      double elapsedUs =
          std::chrono::duration<double, std::micro>(end - start).count();
      timingsUs.push_back(elapsedUs);
    }

    if (timingsUs.empty()) {
      result.success = false;
      result.errorMessage = "No timing samples collected";
      m_results.push_back(result);
      return result;
    }

    std::sort(timingsUs.begin(), timingsUs.end());

    double sum = std::accumulate(timingsUs.begin(), timingsUs.end(), 0.0);
    result.meanUs = sum / timingsUs.size();
    result.minUs = timingsUs.front();
    result.maxUs = timingsUs.back();
    result.medianUs = timingsUs[timingsUs.size() / 2];

    double sqDiffSum = 0.0;
    for (double t : timingsUs) {
      sqDiffSum += (t - result.meanUs) * (t - result.meanUs);
    }
    result.stddevUs = std::sqrt(sqDiffSum / timingsUs.size());

    // Throughput in Megabytes per second: (bytes / (meanUs / 1e6)) / (1024 * 1024)
    if (result.meanUs > 0.0 && fileSizeBytes > 0) {
      double seconds = result.meanUs / 1e6;
      double megabytes = static_cast<double>(fileSizeBytes) / (1024.0 * 1024.0);
      result.throughputMBs = megabytes / seconds;
    }

    result.success = true;
    m_results.push_back(result);
    return result;
  }

  const std::vector<BenchmarkResult> &results() const { return m_results; }

  void printSummaryTable() const {
    std::cout << "\n";
    std::cout << "========================================================================================================\n";
    std::cout << "                                  PARSING PERFORMANCE BENCHMARK RESULTS                                 \n";
    std::cout << "========================================================================================================\n";
    std::cout << std::left
              << std::setw(12) << "Dataset"
              << std::setw(8)  << "Format"
              << std::setw(18) << "Parser"
              << std::right
              << std::setw(12) << "Size (KB)"
              << std::setw(14) << "Mean Time"
              << std::setw(12) << "Median"
              << std::setw(12) << "Min"
              << std::setw(12) << "Max"
              << std::setw(16) << "Throughput"
              << "\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";

    for (const auto &r : m_results) {
      if (!r.success) {
        std::cout << std::left
                  << std::setw(12) << r.datasetName
                  << std::setw(8)  << r.format
                  << std::setw(18) << r.parserName
                  << " [FAILED: " << r.errorMessage << "]\n";
        continue;
      }

      double sizeKb = static_cast<double>(r.fileSizeBytes) / 1024.0;
      std::string timeStr = formatTime(r.meanUs);
      std::string medStr = formatTime(r.medianUs);
      std::string minStr = formatTime(r.minUs);
      std::string maxStr = formatTime(r.maxUs);
      std::string tputStr = formatThroughput(r.throughputMBs);

      std::cout << std::left
                << std::setw(12) << r.datasetName
                << std::setw(8)  << r.format
                << std::setw(18) << r.parserName
                << std::right
                << std::setw(12) << std::fixed << std::setprecision(1) << sizeKb
                << std::setw(14) << timeStr
                << std::setw(12) << medStr
                << std::setw(12) << minStr
                << std::setw(12) << maxStr
                << std::setw(16) << tputStr
                << "\n";
    }
    std::cout << "========================================================================================================\n\n";
  }

  std::string toMarkdown() const {
    std::ostringstream ss;
    ss << "### Parsing Benchmark Summary\n\n";
    ss << "| Dataset | Format | Parser | Size | Mean Time | Median | Min | Max | Throughput |\n";
    ss << "|---|---|---|---|---|---|---|---|---|\n";

    for (const auto &r : m_results) {
      if (!r.success) {
        ss << "| " << r.datasetName << " | " << r.format << " | " << r.parserName
           << " | " << r.fileSizeBytes << " B | *Failed* | - | - | - | - |\n";
        continue;
      }
      double sizeKb = static_cast<double>(r.fileSizeBytes) / 1024.0;
      ss << "| " << r.datasetName << " | "
         << "**" << r.format << "** | "
         << r.parserName << " | "
         << std::fixed << std::setprecision(1) << sizeKb << " KB | "
         << formatTime(r.meanUs) << " | "
         << formatTime(r.medianUs) << " | "
         << formatTime(r.minUs) << " | "
         << formatTime(r.maxUs) << " | "
         << "**" << formatThroughput(r.throughputMBs) << "** |\n";
    }
    ss << "\n";
    return ss.str();
  }

  void exportJson(const std::string &path) const {
    std::ofstream out(path);
    if (!out) return;
    out << "[\n";
    for (size_t i = 0; i < m_results.size(); ++i) {
      const auto &r = m_results[i];
      out << "  {\n";
      out << "    \"dataset\": \"" << r.datasetName << "\",\n";
      out << "    \"format\": \"" << r.format << "\",\n";
      out << "    \"parser\": \"" << r.parserName << "\",\n";
      out << "    \"fileSizeBytes\": " << r.fileSizeBytes << ",\n";
      out << "    \"iterations\": " << r.iterations << ",\n";
      out << "    \"meanUs\": " << r.meanUs << ",\n";
      out << "    \"medianUs\": " << r.medianUs << ",\n";
      out << "    \"minUs\": " << r.minUs << ",\n";
      out << "    \"maxUs\": " << r.maxUs << ",\n";
      out << "    \"stddevUs\": " << r.stddevUs << ",\n";
      out << "    \"throughputMBs\": " << r.throughputMBs << ",\n";
      out << "    \"success\": " << (r.success ? "true" : "false") << "\n";
      out << "  }" << (i + 1 < m_results.size() ? "," : "") << "\n";
    }
    out << "]\n";
  }

private:
  static std::string formatTime(double us) {
    std::ostringstream ss;
    if (us < 1000.0) {
      ss << std::fixed << std::setprecision(2) << us << " us";
    } else if (us < 1000000.0) {
      ss << std::fixed << std::setprecision(2) << (us / 1000.0) << " ms";
    } else {
      ss << std::fixed << std::setprecision(2) << (us / 1000000.0) << " s";
    }
    return ss.str();
  }

  static std::string formatThroughput(double mbPerSec) {
    std::ostringstream ss;
    if (mbPerSec >= 1000.0) {
      ss << std::fixed << std::setprecision(2) << (mbPerSec / 1024.0) << " GB/s";
    } else {
      ss << std::fixed << std::setprecision(2) << mbPerSec << " MB/s";
    }
    return ss.str();
  }

  BenchmarkConfig m_config;
  std::vector<BenchmarkResult> m_results;
};

} // namespace benchmark

#endif // BENCHMARK_HARNESS_HPP
