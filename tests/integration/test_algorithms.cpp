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
    base_path_ = "/zdata/projects/sontv/sun/tests/fixtures/igv/";
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

// Note: ArraySum, LinearSearch, BinarySearch, BubbleSort, and MatrixMultiply
// tests require array support which is more complex. These will be added
// once basic scalar tests are working.

// Loop-free tests for verifying basic interpreter functionality

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
