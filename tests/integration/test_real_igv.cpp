#include <gtest/gtest.h>

#include <memory>
#include <set>

#include "suntv/igv/parser.hpp"
#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"

using namespace sun;

/**
 * Integration tests with real C2-generated IGV dumps.
 * These test the interpreter with actual HotSpot compiler output.
 */

TEST(RealIGVTest, ListFibonacciOpcodes) {
  // Parse the Fibonacci IGV dump
  IGVParser parser;
  auto graph = parser.Parse(
      "/zdata/projects/sontv/sun/tests/fixtures/igv/Fibonacci.xml");

  ASSERT_NE(graph, nullptr) << "Failed to parse Fibonacci.xml";

  // Collect all unique opcodes in the graph
  std::set<Opcode> opcodes;
  for (auto* node : graph->nodes()) {
    opcodes.insert(node->opcode());
  }

  // Print all opcodes for debugging
  std::cout << "Opcodes found in Fibonacci graph:\n";
  for (Opcode op : opcodes) {
    std::cout << "  - " << OpcodeToString(op) << "\n";
  }

  // This test just lists opcodes - it always passes
  SUCCEED();
}

TEST(RealIGVTest, DISABLED_InterpretFibonacci) {
  // Parse the Fibonacci IGV dump
  IGVParser parser;
  auto graph = parser.Parse(
      "/zdata/projects/sontv/sun/tests/fixtures/igv/Fibonacci.xml");

  ASSERT_NE(graph, nullptr) << "Failed to parse Fibonacci.xml";

  // Create interpreter
  Interpreter interp(*graph);

  // Test fib(5) = 5
  // Note: We need to figure out how to pass parameters
  // auto result = interp.Execute({5});
  // EXPECT_EQ(result.return_value->as_i32(), 5);
}
