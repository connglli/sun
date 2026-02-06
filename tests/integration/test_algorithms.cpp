#include <gtest/gtest.h>

#include "suntv/igv/parser.hpp"
#include "suntv/interp/interpreter.hpp"
#include "suntv/interp/value.hpp"
#include "suntv/ir/graph.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

// Test fixture for algorithm integration tests
class AlgorithmTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Base path for IGV fixtures
#ifndef SUN_TEST_FIXTURE_DIR
#define SUN_TEST_FIXTURE_DIR "tests/fixtures"
#endif
    base_path_ = std::string(SUN_TEST_FIXTURE_DIR) + "/igv/";
  }

  // Helper to load and execute a graph
  Outcome ExecuteGraph(const std::string& filename,
                       const std::vector<Value>& inputs) {
    IGVParser parser;
    std::string full_path = base_path_ + filename;
    auto graph = parser.Parse(full_path);

    Interpreter interp(*graph);
    return interp.Execute(inputs);
  }

  // Helper to load and execute a graph with initial heap
  Outcome ExecuteGraphWithHeap(const std::string& filename,
                               const std::vector<Value>& inputs,
                               const ConcreteHeap& initial_heap) {
    IGVParser parser;
    std::string full_path = base_path_ + filename;
    auto graph = parser.Parse(full_path);

    Interpreter interp(*graph);
    return interp.ExecuteWithHeap(inputs, initial_heap);
  }

  // Helper to create an array in the heap
  Ref CreateIntArray(ConcreteHeap& heap, const std::vector<int32_t>& values) {
    Ref arr_ref = heap.AllocateArray(values.size());
    for (size_t i = 0; i < values.size(); ++i) {
      heap.WriteArray(arr_ref, i, Value::MakeI32(values[i]));
    }
    return arr_ref;
  }

  std::string base_path_;
};

// Test Fibonacci computation
TEST_F(AlgorithmTest, Fibonacci) {
  // fib(0) = 0
  {
    auto outcome = ExecuteGraph("Fibonacci.xml", {Value::MakeI32(0)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    if (outcome.return_value->as_i32() != 0) {
      std::cout << "fib(0) returned: " << outcome.return_value->as_i32()
                << " (expected 0)\n";
    }
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }

  // fib(1) = 1
  {
    auto outcome = ExecuteGraph("Fibonacci.xml", {Value::MakeI32(1)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // fib(5) = 5
  {
    auto outcome = ExecuteGraph("Fibonacci.xml", {Value::MakeI32(5)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // fib(10) = 55
  {
    auto outcome = ExecuteGraph("Fibonacci.xml", {Value::MakeI32(10)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 55);
  }
}

// Test Factorial computation
TEST_F(AlgorithmTest, Factorial) {
  // fact(0) = 1
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(0)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // fact(1) = 1
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(1)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // fact(3) = 6
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(3)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 6);
  }

  // fact(4) = 24
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(4)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 24);
  }

  // fact(5) = 120
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(5)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 120);
  }

  // fact(10) = 3628800
  {
    auto outcome = ExecuteGraph("Factorial.xml", {Value::MakeI32(10)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 3628800);
  }
}

// Test GCD computation
TEST_F(AlgorithmTest, GCD) {
  // gcd(48, 18) = 6
  {
    auto outcome =
        ExecuteGraph("GCD.xml", {Value::MakeI32(48), Value::MakeI32(18)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 6);
  }

  // gcd(100, 35) = 5
  {
    auto outcome =
        ExecuteGraph("GCD.xml", {Value::MakeI32(100), Value::MakeI32(35)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // gcd(17, 19) = 1 (coprime)
  {
    auto outcome =
        ExecuteGraph("GCD.xml", {Value::MakeI32(17), Value::MakeI32(19)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }
}

// Test Power computation
TEST_F(AlgorithmTest, Power) {
  // 2^0 = 1
  {
    auto outcome =
        ExecuteGraph("Power.xml", {Value::MakeI32(2), Value::MakeI32(0)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // 2^10 = 1024
  {
    auto outcome =
        ExecuteGraph("Power.xml", {Value::MakeI32(2), Value::MakeI32(10)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1024);
  }

  // 5^3 = 125
  {
    auto outcome =
        ExecuteGraph("Power.xml", {Value::MakeI32(5), Value::MakeI32(3)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 125);
  }
}

// Test IsPrime computation
TEST_F(AlgorithmTest, IsPrime) {
  // isPrime(2) = true (1)
  {
    auto outcome = ExecuteGraph("IsPrime.xml", {Value::MakeI32(2)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);  // true = 1
  }

  // isPrime(17) = true (1)
  {
    auto outcome = ExecuteGraph("IsPrime.xml", {Value::MakeI32(17)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // isPrime(16) = false (0)
  {
    auto outcome = ExecuteGraph("IsPrime.xml", {Value::MakeI32(16)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);  // false = 0
  }
}

// ============================================================================
// Array-based algorithm tests
// ============================================================================
//
// NOTE: These tests verify that the interpreter can parse and load complex
// array-based algorithm IGV files. Full execution testing requires array input
// support, which needs additional infrastructure to:
// 1. Pre-populate the heap with array data
// 2. Pass array references as parameters
//
// For now, we test that:
// - The IGV files parse successfully
// - The graphs can be loaded without errors
// - Unknown opcodes (LoadRange, RangeCheck) are handled gracefully
//
// TODO: Add full execution tests once array input support is implemented

// Helper to test that a graph parses successfully
void TestGraphParses(const std::string& filename) {
  IGVParser parser;
  std::string full_path =
      std::string(SUN_TEST_FIXTURE_DIR) + "/igv/" + filename;

  // This should not throw
  auto graph = parser.Parse(full_path);

  // Verify graph is not null and has nodes
  ASSERT_NE(graph, nullptr);
  ASSERT_GT(graph->nodes().size(), 0);
}

// Test ArraySum - sum all elements in an array
// Java signature: public static int compute(int[] arr)
TEST_F(AlgorithmTest, ArraySum_ParsesSuccessfully) {
  // Verify the IGV file parses without errors
  TestGraphParses("ArraySum.xml");
}

// Test LinearSearch - find element in array
// Java signature: public static int compute(int[] arr, int target)
TEST_F(AlgorithmTest, LinearSearch_ParsesSuccessfully) {
  // Verify the IGV file parses without errors
  TestGraphParses("LinearSearch.xml");
}

// Test BinarySearch - find element in sorted array
// Java signature: public static int compute(int[] arr, int target)
TEST_F(AlgorithmTest, BinarySearch_ParsesSuccessfully) {
  // Verify the IGV file parses without errors
  TestGraphParses("BinarySearch.xml");
}

// Test BubbleSort - sort array in place
// Java signature: public static void compute(int[] arr)
TEST_F(AlgorithmTest, BubbleSort_ParsesSuccessfully) {
  // Verify the IGV file parses without errors
  TestGraphParses("BubbleSort.xml");
}

// Test MatrixMultiply - multiply two 2D matrices
// Java signature: public static int[][] compute(int[][] a, int[][] b)
TEST_F(AlgorithmTest, MatrixMultiply_ParsesSuccessfully) {
  // Verify the IGV file parses without errors
  TestGraphParses("MatrixMultiply.xml");
}

// ============================================================================
// Full execution tests for array-based algorithms
// ============================================================================

// Test ArraySum with actual execution
TEST_F(AlgorithmTest, ArraySum_FullExecution) {
  // Test 1: Sum of {1, 2, 3, 4, 5} = 15
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {1, 2, 3, 4, 5});

    auto outcome =
        ExecuteGraphWithHeap("ArraySum.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 15);
  }

  // Test 2: Sum of {10, 20, 30} = 60
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {10, 20, 30});

    auto outcome =
        ExecuteGraphWithHeap("ArraySum.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 60);
  }

  // Test 3: Sum of single element {42} = 42
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {42});

    auto outcome =
        ExecuteGraphWithHeap("ArraySum.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 42);
  }

  // Test 4: Sum of empty array {} = 0
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {});

    auto outcome =
        ExecuteGraphWithHeap("ArraySum.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }

  // Test 5: Sum with negative numbers {-5, 10, -3, 8} = 10
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {-5, 10, -3, 8});

    auto outcome =
        ExecuteGraphWithHeap("ArraySum.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 10);
  }
}

// Test LinearSearch with actual execution
TEST_F(AlgorithmTest, LinearSearch_FullExecution) {
  // Test 1: Find element in array {10, 23, 45, 70, 11, 15}, search for 70
  // Expected: index 3
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {10, 23, 45, 70, 11, 15});

    auto outcome = ExecuteGraphWithHeap(
        "LinearSearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(70)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 3);
  }

  // Test 2: Find first element
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {10, 23, 45, 70, 11, 15});

    auto outcome = ExecuteGraphWithHeap(
        "LinearSearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(10)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }

  // Test 3: Find last element
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {10, 23, 45, 70, 11, 15});

    auto outcome = ExecuteGraphWithHeap(
        "LinearSearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(15)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // Test 4: Element not found, should return -1
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {10, 23, 45, 70, 11, 15});

    auto outcome = ExecuteGraphWithHeap(
        "LinearSearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(99)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), -1);
  }

  // Test 5: Search in single-element array (found)
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {42});

    auto outcome = ExecuteGraphWithHeap(
        "LinearSearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(42)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }
}

// Test BinarySearch with actual execution
TEST_F(AlgorithmTest, BinarySearch_FullExecution) {
  // Test 1: Find element in sorted array {2, 5, 8, 12, 16, 23, 38, 45, 56, 67,
  // 78} Search for 23, expected: index 5
  {
    ConcreteHeap heap;
    Ref arr_ref =
        CreateIntArray(heap, {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78});

    auto outcome = ExecuteGraphWithHeap(
        "BinarySearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(23)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // Test 2: Find first element
  {
    ConcreteHeap heap;
    Ref arr_ref =
        CreateIntArray(heap, {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78});

    auto outcome = ExecuteGraphWithHeap(
        "BinarySearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(2)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }

  // Test 3: Find last element
  {
    ConcreteHeap heap;
    Ref arr_ref =
        CreateIntArray(heap, {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78});

    auto outcome = ExecuteGraphWithHeap(
        "BinarySearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(78)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 10);
  }

  // Test 4: Element not found, should return -1
  {
    ConcreteHeap heap;
    Ref arr_ref =
        CreateIntArray(heap, {2, 5, 8, 12, 16, 23, 38, 45, 56, 67, 78});

    auto outcome = ExecuteGraphWithHeap(
        "BinarySearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(99)},
        heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), -1);
  }

  // Test 5: Search in small array
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {1, 3, 5, 7, 9});

    auto outcome = ExecuteGraphWithHeap(
        "BinarySearch.xml", {Value::MakeRef(arr_ref), Value::MakeI32(5)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 2);
  }
}

// Test BubbleSort with actual execution
TEST_F(AlgorithmTest, BubbleSort_FullExecution) {
  // Test 1: Sort {64, 34, 25, 12, 22, 11, 90}
  // Expected: {11, 12, 22, 25, 34, 64, 90}
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {64, 34, 25, 12, 22, 11, 90});

    auto outcome =
        ExecuteGraphWithHeap("BubbleSort.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);

    // Verify array is sorted in-place
    std::vector<Value> sorted = outcome.heap.GetArrayContents(arr_ref);
    ASSERT_EQ(sorted.size(), 7);
    EXPECT_EQ(sorted[0].as_i32(), 11);
    EXPECT_EQ(sorted[1].as_i32(), 12);
    EXPECT_EQ(sorted[2].as_i32(), 22);
    EXPECT_EQ(sorted[3].as_i32(), 25);
    EXPECT_EQ(sorted[4].as_i32(), 34);
    EXPECT_EQ(sorted[5].as_i32(), 64);
    EXPECT_EQ(sorted[6].as_i32(), 90);
  }

  // Test 2: Sort already sorted array {1, 2, 3, 4, 5}
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {1, 2, 3, 4, 5});

    auto outcome =
        ExecuteGraphWithHeap("BubbleSort.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);

    std::vector<Value> sorted = outcome.heap.GetArrayContents(arr_ref);
    ASSERT_EQ(sorted.size(), 5);
    EXPECT_EQ(sorted[0].as_i32(), 1);
    EXPECT_EQ(sorted[1].as_i32(), 2);
    EXPECT_EQ(sorted[2].as_i32(), 3);
    EXPECT_EQ(sorted[3].as_i32(), 4);
    EXPECT_EQ(sorted[4].as_i32(), 5);
  }

  // Test 3: Sort reverse-sorted array {5, 4, 3, 2, 1}
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {5, 4, 3, 2, 1});

    auto outcome =
        ExecuteGraphWithHeap("BubbleSort.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);

    std::vector<Value> sorted = outcome.heap.GetArrayContents(arr_ref);
    ASSERT_EQ(sorted.size(), 5);
    EXPECT_EQ(sorted[0].as_i32(), 1);
    EXPECT_EQ(sorted[1].as_i32(), 2);
    EXPECT_EQ(sorted[2].as_i32(), 3);
    EXPECT_EQ(sorted[3].as_i32(), 4);
    EXPECT_EQ(sorted[4].as_i32(), 5);
  }

  // Test 4: Sort array with duplicates {3, 1, 4, 1, 5, 9, 2, 6, 5}
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {3, 1, 4, 1, 5, 9, 2, 6, 5});

    auto outcome =
        ExecuteGraphWithHeap("BubbleSort.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);

    std::vector<Value> sorted = outcome.heap.GetArrayContents(arr_ref);
    ASSERT_EQ(sorted.size(), 9);
    EXPECT_EQ(sorted[0].as_i32(), 1);
    EXPECT_EQ(sorted[1].as_i32(), 1);
    EXPECT_EQ(sorted[2].as_i32(), 2);
    EXPECT_EQ(sorted[3].as_i32(), 3);
    EXPECT_EQ(sorted[4].as_i32(), 4);
    EXPECT_EQ(sorted[5].as_i32(), 5);
    EXPECT_EQ(sorted[6].as_i32(), 5);
    EXPECT_EQ(sorted[7].as_i32(), 6);
    EXPECT_EQ(sorted[8].as_i32(), 9);
  }

  // Test 5: Sort single element array
  {
    ConcreteHeap heap;
    Ref arr_ref = CreateIntArray(heap, {42});

    auto outcome =
        ExecuteGraphWithHeap("BubbleSort.xml", {Value::MakeRef(arr_ref)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);

    std::vector<Value> sorted = outcome.heap.GetArrayContents(arr_ref);
    ASSERT_EQ(sorted.size(), 1);
    EXPECT_EQ(sorted[0].as_i32(), 42);
  }
}

// Test MatrixMultiply with actual execution
// Note: This test is more complex as it requires 2D arrays
TEST_F(AlgorithmTest, MatrixMultiply_FullExecution) {
  // For now, we'll test that the function executes and returns a valid
  // reference Full 2D array support requires more complex heap setup

  // Test: 2x2 matrix multiplication
  // a = {{1, 2}, {3, 4}}
  // b = {{5, 6}, {7, 8}}
  // Expected result: {{19, 22}, {43, 50}}
  {
    ConcreteHeap heap;

    // Create first matrix a[2][2]
    Ref row0_a = CreateIntArray(heap, {1, 2});
    Ref row1_a = CreateIntArray(heap, {3, 4});
    Ref matrix_a = heap.AllocateArray(2);
    heap.WriteArray(matrix_a, 0, Value::MakeRef(row0_a));
    heap.WriteArray(matrix_a, 1, Value::MakeRef(row1_a));

    // Create second matrix b[2][2]
    Ref row0_b = CreateIntArray(heap, {5, 6});
    Ref row1_b = CreateIntArray(heap, {7, 8});
    Ref matrix_b = heap.AllocateArray(2);
    heap.WriteArray(matrix_b, 0, Value::MakeRef(row0_b));
    heap.WriteArray(matrix_b, 1, Value::MakeRef(row1_b));

    auto outcome = ExecuteGraphWithHeap(
        "MatrixMultiply.xml",
        {Value::MakeRef(matrix_a), Value::MakeRef(matrix_b)}, heap);

    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());

    // Verify result is a valid reference
    EXPECT_TRUE(outcome.return_value->is_ref());

    // Get the result matrix and verify its contents
    Ref result_ref = outcome.return_value->as_ref();

    // Result should be a 2x2 matrix
    int32_t result_len = outcome.heap.ArrayLength(result_ref);
    EXPECT_EQ(result_len, 2);

    // Get first row
    Value row0_val = outcome.heap.ReadArray(result_ref, 0);
    ASSERT_TRUE(row0_val.is_ref());
    Ref row0_ref = row0_val.as_ref();
    std::vector<Value> row0 = outcome.heap.GetArrayContents(row0_ref);
    ASSERT_EQ(row0.size(), 2);
    EXPECT_EQ(row0[0].as_i32(), 19);  // 1*5 + 2*7 = 19
    EXPECT_EQ(row0[1].as_i32(), 22);  // 1*6 + 2*8 = 22

    // Get second row
    Value row1_val = outcome.heap.ReadArray(result_ref, 1);
    ASSERT_TRUE(row1_val.is_ref());
    Ref row1_ref = row1_val.as_ref();
    std::vector<Value> row1 = outcome.heap.GetArrayContents(row1_ref);
    ASSERT_EQ(row1.size(), 2);
    EXPECT_EQ(row1[0].as_i32(), 43);  // 3*5 + 4*7 = 43
    EXPECT_EQ(row1[1].as_i32(), 50);  // 3*6 + 4*8 = 50
  }
}

// ============================================================================
// Loop-free tests for verifying basic interpreter functionality
// ============================================================================

// Test Max computation
TEST_F(AlgorithmTest, Max) {
  // Enable debug logging for this test
  Logger::SetLevel(LogLevel::DEBUG);

  // max(5, 10) = 10
  {
    auto outcome =
        ExecuteGraph("Max.xml", {Value::MakeI32(5), Value::MakeI32(10)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 10);
  }

  // max(15, 3) = 15
  {
    auto outcome =
        ExecuteGraph("Max.xml", {Value::MakeI32(15), Value::MakeI32(3)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 15);
  }

  // max(-5, -10) = -5
  {
    auto outcome =
        ExecuteGraph("Max.xml", {Value::MakeI32(-5), Value::MakeI32(-10)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), -5);
  }

  // Reset log level
  Logger::SetLevel(LogLevel::INFO);
}

// Test Abs computation
TEST_F(AlgorithmTest, Abs) {
  // abs(-5) = 5
  {
    auto outcome = ExecuteGraph("Abs.xml", {Value::MakeI32(-5)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // abs(7) = 7
  {
    auto outcome = ExecuteGraph("Abs.xml", {Value::MakeI32(7)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 7);
  }

  // abs(0) = 0
  {
    auto outcome = ExecuteGraph("Abs.xml", {Value::MakeI32(0)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }
}

// Test Sign computation
TEST_F(AlgorithmTest, Sign) {
  // sign(5) = 1
  {
    auto outcome = ExecuteGraph("Sign.xml", {Value::MakeI32(5)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // sign(-7) = -1
  {
    auto outcome = ExecuteGraph("Sign.xml", {Value::MakeI32(-7)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), -1);
  }

  // sign(0) = 0
  {
    auto outcome = ExecuteGraph("Sign.xml", {Value::MakeI32(0)});
    ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
    ASSERT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }
}

}  // namespace sun
