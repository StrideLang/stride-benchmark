#ifndef JSON_BENCH_HPP
#define JSON_BENCH_HPP

#include <fstream>
#include <sstream>
#include <string>

#if defined(BENCHMARK_HAS_SIMDJSON)
#include "simdjson.h"
#endif

#if defined(BENCHMARK_HAS_RAPIDJSON)
#include "rapidjson/document.h"
#endif

#if defined(BENCHMARK_HAS_NLOHMANN)
#include <nlohmann/json.hpp>
#endif

namespace benchmark {

class JsonBenchmarkParser {
public:
  static std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }

#if defined(BENCHMARK_HAS_SIMDJSON)
  static bool parseSimdJsonFile(const std::string &path) {
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.load(path).get(doc);
    return !error;
  }

  static bool parseSimdJsonBuffer(const simdjson::padded_string &buffer) {
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(buffer).get(doc);
    return !error;
  }
#endif

#if defined(BENCHMARK_HAS_RAPIDJSON)
  static bool parseRapidJson(const std::string &content) {
    rapidjson::Document doc;
    doc.Parse(content.c_str());
    return !doc.HasParseError();
  }
#endif

#if defined(BENCHMARK_HAS_NLOHMANN)
  static bool parseNlohmannJson(const std::string &content) {
    auto j = nlohmann::json::parse(content, nullptr, false);
    return !j.is_discarded();
  }
#endif
};

} // namespace benchmark

#endif // JSON_BENCH_HPP
