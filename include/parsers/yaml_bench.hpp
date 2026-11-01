#ifndef YAML_BENCH_HPP
#define YAML_BENCH_HPP

#include <fstream>
#include <sstream>
#include <string>

#if defined(BENCHMARK_HAS_RAPIDYAML)
#include <ryml.hpp>
#endif

#if defined(BENCHMARK_HAS_YAMLCPP)
#include <yaml-cpp/yaml.h>
#endif

namespace benchmark {

class YamlBenchmarkParser {
public:
  static std::string readFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
  }

#if defined(BENCHMARK_HAS_RAPIDYAML)
  static bool parseRapidYaml(const std::string &content) {
    // ryml::parse_in_arena parses read-only string without modifying the input
    ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(content));
    return !tree.empty();
  }
#endif

#if defined(BENCHMARK_HAS_YAMLCPP)
  static bool parseYamlCpp(const std::string &content) {
    YAML::Node node = YAML::Load(content);
    return node.IsDefined();
  }

  static bool parseYamlCppFile(const std::string &path) {
    YAML::Node node = YAML::LoadFile(path);
    return node.IsDefined();
  }
#endif
};

} // namespace benchmark

#endif // YAML_BENCH_HPP
