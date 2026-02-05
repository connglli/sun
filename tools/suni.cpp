#include <iostream>

#include "suntv/igv/parser.hpp"
#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: suni <graph.igv> [args...]\n";
    return 1;
  }

  std::cout << "suni (stub): " << argv[1] << "\n";
  return 0;
}
