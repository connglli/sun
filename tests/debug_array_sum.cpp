#include <iostream>

#include "suntv/igv/parser.hpp"
#include "suntv/interp/heap.hpp"
#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"
#include "suntv/util/logging.hpp"

using namespace sun;

int main() {
  // Enable debug logging
  Logger::SetLevel(LogLevel::DEBUG);

  // Parse the graph
  IGVParser parser;
  auto graph = parser.Parse("tests/fixtures/igv/ArraySum.xml");

  std::cout << "=== Graph parsed successfully ===" << std::endl;
  std::cout << "Number of nodes: " << graph->nodes().size() << std::endl;

  // Create test heap with array
  ConcreteHeap heap;
  Ref arr_ref = heap.AllocateArray(5);
  heap.WriteArray(arr_ref, 0, Value::MakeI32(1));
  heap.WriteArray(arr_ref, 1, Value::MakeI32(2));
  heap.WriteArray(arr_ref, 2, Value::MakeI32(3));
  heap.WriteArray(arr_ref, 3, Value::MakeI32(4));
  heap.WriteArray(arr_ref, 4, Value::MakeI32(5));

  std::cout << "=== Heap initialized ===" << std::endl;
  std::cout << heap.Dump() << std::endl;

  // Execute
  try {
    Interpreter interp(*graph);
    std::cout << "=== Starting execution ===" << std::endl;
    Outcome outcome = interp.ExecuteWithHeap({Value::MakeRef(arr_ref)}, heap);

    std::cout << "=== Execution completed ===" << std::endl;
    std::cout << "Outcome: " << outcome.ToString() << std::endl;

    if (outcome.return_value.has_value()) {
      std::cout << "Return value: " << outcome.return_value->as_i32()
                << std::endl;
    }
  } catch (const std::exception& e) {
    std::cout << "=== Exception caught ===" << std::endl;
    std::cout << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
