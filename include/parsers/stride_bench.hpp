#ifndef STRIDE_BENCH_HPP
#define STRIDE_BENCH_HPP

#include "stride/parser/ast.h"
#include <string>

namespace benchmark {

class StrideBenchmarkParser {
public:
  static strd::ASTNode parseFile(const std::string &path) {
    return strd::AST::parseFile(path.c_str());
  }
};

} // namespace benchmark

#endif // STRIDE_BENCH_HPP
