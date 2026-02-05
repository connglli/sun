#include <iostream>
#include <string>
#include <vector>

#include "suntv/igv/parser.hpp"
#include "suntv/interp/interpreter.hpp"
#include "suntv/interp/value.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

// Parse command-line integer argument to Value
Value ParseIntArg(const std::string& arg) {
  try {
    // Try int32 first
    int32_t val32 = std::stoi(arg);
    return Value::MakeI32(val32);
  } catch (const std::out_of_range&) {
    // If out of range for int32, try int64
    int64_t val64 = std::stoll(arg);
    return Value::MakeI64(val64);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: suni <graph.igv> [args...]\n";
    std::cerr << "  <graph.igv>  Path to IGV graph file\n";
    std::cerr << "  [args...]    Integer arguments to pass to the graph\n";
    return 1;
  }

  std::string graph_path = argv[1];

  // Parse input arguments
  std::vector<Value> inputs;
  for (int i = 2; i < argc; ++i) {
    try {
      Value v = ParseIntArg(argv[i]);
      inputs.push_back(v);
    } catch (const std::exception& e) {
      std::cerr << "Error: Failed to parse argument '" << argv[i]
                << "' as integer: " << e.what() << "\n";
      return 1;
    }
  }

  // Parse IGV graph
  IGVParser parser;
  std::unique_ptr<Graph> graph;
  try {
    graph = parser.Parse(graph_path);
    if (!graph) {
      std::cerr << "Error: Failed to parse IGV file (null graph returned)\n";
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: Failed to parse IGV file '" << graph_path
              << "': " << e.what() << "\n";
    return 1;
  }

  // Execute graph
  Interpreter interp(*graph);
  Outcome outcome;
  try {
    outcome = interp.Execute(inputs);
  } catch (const std::exception& e) {
    std::cerr << "Error: Interpreter failed: " << e.what() << "\n";
    return 1;
  }

  // Print outcome
  std::cout << outcome.ToString() << "\n";

  // Exit code: 0 for Return, 1 for Throw
  return (outcome.kind == Outcome::Kind::kReturn) ? 0 : 1;
}
