#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"

using namespace sun;

/**
 * Complex algorithmic tests for the interpreter.
 * These tests verify that the interpreter can handle loops and complex
 * control flow correctly.
 */

// Test 1: Iterative Fibonacci
// int fib(int n) {
//   if (n <= 1) return n;
//   int a = 0, b = 1;
//   for (int i = 2; i <= n; i++) {
//     int tmp = a + b;
//     a = b;
//     b = tmp;
//   }
//   return b;
// }
TEST(AlgorithmTest, IterativeFibonacci) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Parameter: n
  Node* parm_n = g.AddNode(2, Opcode::kParm);
  parm_n->set_prop("index", static_cast<int32_t>(0));

  // Check if n <= 1 (base case)
  Node* const_1 = g.AddNode(3, Opcode::kConI);
  const_1->set_prop("value", static_cast<int32_t>(1));

  Node* cmp_base = g.AddNode(4, Opcode::kCmpI);
  cmp_base->set_input(0, parm_n);
  cmp_base->set_input(1, const_1);

  // Bool: n <= 1 (LE mask = LT|EQ = 1|2 = 3)
  Node* bool_base = g.AddNode(5, Opcode::kBool);
  bool_base->set_input(0, cmp_base);
  bool_base->set_prop("mask", static_cast<int32_t>(3));  // LE

  // If node for base case
  Node* if_base = g.AddNode(6, Opcode::kIf);
  if_base->set_input(0, start);
  if_base->set_input(1, bool_base);

  Node* if_true_base = g.AddNode(7, Opcode::kIfTrue);
  if_true_base->set_input(0, if_base);

  Node* if_false_base = g.AddNode(8, Opcode::kIfFalse);
  if_false_base->set_input(0, if_base);

  // Base case: return n
  Node* ret_base = g.AddNode(9, Opcode::kReturn);
  ret_base->set_input(0, if_true_base);
  ret_base->set_input(1, parm_n);

  // Loop case: initialize a=0, b=1, i=2
  Node* const_0 = g.AddNode(10, Opcode::kConI);
  const_0->set_prop("value", static_cast<int32_t>(0));

  Node* const_2 = g.AddNode(11, Opcode::kConI);
  const_2->set_prop("value", static_cast<int32_t>(2));

  // Loop header (Region node that merges initialization and loop back edge)
  Node* loop_header = g.AddNode(12, Opcode::kRegion);
  loop_header->set_input(0, if_false_base);  // First entry

  // Phi nodes for loop variables: a, b, i
  Node* phi_a = g.AddNode(13, Opcode::kPhi);
  phi_a->set_input(0, loop_header);
  phi_a->set_input(1, const_0);  // Initial: a = 0

  Node* phi_b = g.AddNode(14, Opcode::kPhi);
  phi_b->set_input(0, loop_header);
  phi_b->set_input(1, const_1);  // Initial: b = 1

  Node* phi_i = g.AddNode(15, Opcode::kPhi);
  phi_i->set_input(0, loop_header);
  phi_i->set_input(1, const_2);  // Initial: i = 2

  // Loop body: tmp = a + b
  Node* add_tmp = g.AddNode(16, Opcode::kAddI);
  add_tmp->set_input(0, phi_a);
  add_tmp->set_input(1, phi_b);

  // Next iteration values: a' = b, b' = tmp
  // (phi_b and add_tmp will be used in back edge)

  // Increment i: i' = i + 1
  Node* add_i = g.AddNode(17, Opcode::kAddI);
  add_i->set_input(0, phi_i);
  add_i->set_input(1, const_1);

  // Loop condition: i <= n
  Node* cmp_loop = g.AddNode(18, Opcode::kCmpI);
  cmp_loop->set_input(0, add_i);
  cmp_loop->set_input(1, parm_n);

  Node* bool_loop = g.AddNode(19, Opcode::kBool);
  bool_loop->set_input(0, cmp_loop);
  bool_loop->set_prop("mask", static_cast<int32_t>(3));  // LE

  // If for loop condition
  Node* if_loop = g.AddNode(20, Opcode::kIf);
  if_loop->set_input(0, loop_header);
  if_loop->set_input(1, bool_loop);

  Node* if_true_loop = g.AddNode(21, Opcode::kIfTrue);
  if_true_loop->set_input(0, if_loop);

  Node* if_false_loop = g.AddNode(22, Opcode::kIfFalse);
  if_false_loop->set_input(0, if_loop);

  // Back edge: connect to loop header
  loop_header->add_input(if_true_loop);  // Second input (back edge)

  // Update phi nodes with back edge values
  phi_a->add_input(phi_b);    // a = b (previous iteration)
  phi_b->add_input(add_tmp);  // b = tmp
  phi_i->add_input(add_i);    // i = i + 1

  // Loop exit: return b
  Node* ret_loop = g.AddNode(23, Opcode::kReturn);
  ret_loop->set_input(0, if_false_loop);
  ret_loop->set_input(1, phi_b);

  // Merge returns at root
  Node* region_ret = g.AddNode(24, Opcode::kRegion);
  region_ret->set_input(0, ret_base);
  region_ret->set_input(1, ret_loop);

  root->set_input(0, region_ret);

  // Test cases
  Interpreter interp(g);

  // fib(0) = 0
  {
    std::vector<Value> inputs = {Value::MakeI32(0)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 0);
  }

  // fib(1) = 1
  {
    std::vector<Value> inputs = {Value::MakeI32(1)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // fib(5) = 5 (sequence: 0, 1, 1, 2, 3, 5)
  {
    std::vector<Value> inputs = {Value::MakeI32(5)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // fib(10) = 55
  {
    std::vector<Value> inputs = {Value::MakeI32(10)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_TRUE(outcome.return_value.has_value());
    EXPECT_EQ(outcome.return_value->as_i32(), 55);
  }
}

// Test 2: Factorial
// int factorial(int n) {
//   int result = 1;
//   for (int i = 2; i <= n; i++) {
//     result *= i;
//   }
//   return result;
// }
TEST(AlgorithmTest, Factorial) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Parameter: n
  Node* parm_n = g.AddNode(2, Opcode::kParm);
  parm_n->set_prop("index", static_cast<int32_t>(0));

  // Constants
  Node* const_1 = g.AddNode(3, Opcode::kConI);
  const_1->set_prop("value", static_cast<int32_t>(1));

  Node* const_2 = g.AddNode(4, Opcode::kConI);
  const_2->set_prop("value", static_cast<int32_t>(2));

  // Loop header
  Node* loop_header = g.AddNode(5, Opcode::kRegion);
  loop_header->set_input(0, start);

  // Phi nodes: result, i
  Node* phi_result = g.AddNode(6, Opcode::kPhi);
  phi_result->set_input(0, loop_header);
  phi_result->set_input(1, const_1);  // result = 1

  Node* phi_i = g.AddNode(7, Opcode::kPhi);
  phi_i->set_input(0, loop_header);
  phi_i->set_input(1, const_2);  // i = 2

  // Loop condition: i <= n
  Node* cmp_loop = g.AddNode(8, Opcode::kCmpI);
  cmp_loop->set_input(0, phi_i);
  cmp_loop->set_input(1, parm_n);

  Node* bool_loop = g.AddNode(9, Opcode::kBool);
  bool_loop->set_input(0, cmp_loop);
  bool_loop->set_prop("mask", static_cast<int32_t>(3));  // LE

  Node* if_loop = g.AddNode(10, Opcode::kIf);
  if_loop->set_input(0, loop_header);
  if_loop->set_input(1, bool_loop);

  Node* if_true = g.AddNode(11, Opcode::kIfTrue);
  if_true->set_input(0, if_loop);

  Node* if_false = g.AddNode(12, Opcode::kIfFalse);
  if_false->set_input(0, if_loop);

  // Loop body: result *= i
  Node* mul_result = g.AddNode(13, Opcode::kMulI);
  mul_result->set_input(0, phi_result);
  mul_result->set_input(1, phi_i);

  // i++
  Node* add_i = g.AddNode(14, Opcode::kAddI);
  add_i->set_input(0, phi_i);
  add_i->set_input(1, const_1);

  // Back edge
  loop_header->add_input(if_true);
  phi_result->add_input(mul_result);
  phi_i->add_input(add_i);

  // Return result
  Node* ret = g.AddNode(15, Opcode::kReturn);
  ret->set_input(0, if_false);
  ret->set_input(1, phi_result);

  root->set_input(0, ret);

  // Test cases
  Interpreter interp(g);

  // 0! = 1
  {
    std::vector<Value> inputs = {Value::MakeI32(0)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // 5! = 120
  {
    std::vector<Value> inputs = {Value::MakeI32(5)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 120);
  }

  // 10! = 3628800
  {
    std::vector<Value> inputs = {Value::MakeI32(10)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 3628800);
  }
}

// Test 3: GCD (Euclidean algorithm)
// int gcd(int a, int b) {
//   while (b != 0) {
//     int tmp = b;
//     b = a % b;
//     a = tmp;
//   }
//   return a;
// }
TEST(AlgorithmTest, GCD) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Parameters: a, b
  Node* parm_a = g.AddNode(2, Opcode::kParm);
  parm_a->set_prop("index", static_cast<int32_t>(0));

  Node* parm_b = g.AddNode(3, Opcode::kParm);
  parm_b->set_prop("index", static_cast<int32_t>(1));

  Node* const_0 = g.AddNode(4, Opcode::kConI);
  const_0->set_prop("value", static_cast<int32_t>(0));

  // Loop header
  Node* loop_header = g.AddNode(5, Opcode::kRegion);
  loop_header->set_input(0, start);

  // Phi nodes: a, b
  Node* phi_a = g.AddNode(6, Opcode::kPhi);
  phi_a->set_input(0, loop_header);
  phi_a->set_input(1, parm_a);

  Node* phi_b = g.AddNode(7, Opcode::kPhi);
  phi_b->set_input(0, loop_header);
  phi_b->set_input(1, parm_b);

  // Loop condition: b != 0 (NE mask = LT|GT = 1|4 = 5)
  Node* cmp_loop = g.AddNode(8, Opcode::kCmpI);
  cmp_loop->set_input(0, phi_b);
  cmp_loop->set_input(1, const_0);

  Node* bool_loop = g.AddNode(9, Opcode::kBool);
  bool_loop->set_input(0, cmp_loop);
  bool_loop->set_prop("mask", static_cast<int32_t>(5));  // NE

  Node* if_loop = g.AddNode(10, Opcode::kIf);
  if_loop->set_input(0, loop_header);
  if_loop->set_input(1, bool_loop);

  Node* if_true = g.AddNode(11, Opcode::kIfTrue);
  if_true->set_input(0, if_loop);

  Node* if_false = g.AddNode(12, Opcode::kIfFalse);
  if_false->set_input(0, if_loop);

  // Loop body: tmp = b, b = a % b, a = tmp
  Node* mod_result = g.AddNode(13, Opcode::kModI);
  mod_result->set_input(0, phi_a);
  mod_result->set_input(1, phi_b);

  // Back edge
  loop_header->add_input(if_true);
  phi_a->add_input(phi_b);       // a = tmp (which is b)
  phi_b->add_input(mod_result);  // b = a % b

  // Return a
  Node* ret = g.AddNode(14, Opcode::kReturn);
  ret->set_input(0, if_false);
  ret->set_input(1, phi_a);

  root->set_input(0, ret);

  // Test cases
  Interpreter interp(g);

  // gcd(48, 18) = 6
  {
    std::vector<Value> inputs = {Value::MakeI32(48), Value::MakeI32(18)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 6);
  }

  // gcd(100, 35) = 5
  {
    std::vector<Value> inputs = {Value::MakeI32(100), Value::MakeI32(35)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 5);
  }

  // gcd(17, 13) = 1 (coprime)
  {
    std::vector<Value> inputs = {Value::MakeI32(17), Value::MakeI32(13)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }
}

// Test 4: Power function (iterative)
// int power(int base, int exp) {
//   int result = 1;
//   for (int i = 0; i < exp; i++) {
//     result *= base;
//   }
//   return result;
// }
TEST(AlgorithmTest, Power) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Parameters: base, exp
  Node* parm_base = g.AddNode(2, Opcode::kParm);
  parm_base->set_prop("index", static_cast<int32_t>(0));

  Node* parm_exp = g.AddNode(3, Opcode::kParm);
  parm_exp->set_prop("index", static_cast<int32_t>(1));

  Node* const_0 = g.AddNode(4, Opcode::kConI);
  const_0->set_prop("value", static_cast<int32_t>(0));

  Node* const_1 = g.AddNode(5, Opcode::kConI);
  const_1->set_prop("value", static_cast<int32_t>(1));

  // Loop header
  Node* loop_header = g.AddNode(6, Opcode::kRegion);
  loop_header->set_input(0, start);

  // Phi nodes: result, i
  Node* phi_result = g.AddNode(7, Opcode::kPhi);
  phi_result->set_input(0, loop_header);
  phi_result->set_input(1, const_1);

  Node* phi_i = g.AddNode(8, Opcode::kPhi);
  phi_i->set_input(0, loop_header);
  phi_i->set_input(1, const_0);

  // Loop condition: i < exp (LT mask = 1)
  Node* cmp_loop = g.AddNode(9, Opcode::kCmpI);
  cmp_loop->set_input(0, phi_i);
  cmp_loop->set_input(1, parm_exp);

  Node* bool_loop = g.AddNode(10, Opcode::kBool);
  bool_loop->set_input(0, cmp_loop);
  bool_loop->set_prop("mask", static_cast<int32_t>(1));  // LT

  Node* if_loop = g.AddNode(11, Opcode::kIf);
  if_loop->set_input(0, loop_header);
  if_loop->set_input(1, bool_loop);

  Node* if_true = g.AddNode(12, Opcode::kIfTrue);
  if_true->set_input(0, if_loop);

  Node* if_false = g.AddNode(13, Opcode::kIfFalse);
  if_false->set_input(0, if_loop);

  // Loop body: result *= base
  Node* mul_result = g.AddNode(14, Opcode::kMulI);
  mul_result->set_input(0, phi_result);
  mul_result->set_input(1, parm_base);

  // i++
  Node* add_i = g.AddNode(15, Opcode::kAddI);
  add_i->set_input(0, phi_i);
  add_i->set_input(1, const_1);

  // Back edge
  loop_header->add_input(if_true);
  phi_result->add_input(mul_result);
  phi_i->add_input(add_i);

  // Return
  Node* ret = g.AddNode(16, Opcode::kReturn);
  ret->set_input(0, if_false);
  ret->set_input(1, phi_result);

  root->set_input(0, ret);

  // Test cases
  Interpreter interp(g);

  // 2^0 = 1
  {
    std::vector<Value> inputs = {Value::MakeI32(2), Value::MakeI32(0)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 1);
  }

  // 2^10 = 1024
  {
    std::vector<Value> inputs = {Value::MakeI32(2), Value::MakeI32(10)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 1024);
  }

  // 3^4 = 81
  {
    std::vector<Value> inputs = {Value::MakeI32(3), Value::MakeI32(4)};
    Outcome outcome = interp.Execute(inputs);
    EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
    EXPECT_EQ(outcome.return_value->as_i32(), 81);
  }
}

// TODO: Add more complex tests:
// - Array sum (with memory operations)
// - Array search (linear/binary)
// - Bubble sort (nested loops + array)
// - Matrix multiplication (triple nested loop + 2D arrays)
// - String/array operations (reverse, palindrome check)
