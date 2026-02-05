#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

// Test 1: Simple constant return (return 42)
TEST(InterpreterTest, ConstantReturn) {
  Graph g;

  // Build graph: Root -> Start -> ConI(42) -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);
  Node* con = g.AddNode(2, Opcode::kConI);
  con->set_prop("value", static_cast<int32_t>(42));

  Node* ret = g.AddNode(3, Opcode::kReturn);
  root->set_input(0, ret);   // Root points to Return
  ret->set_input(0, start);  // Control edge
  ret->set_input(1, con);    // Value edge

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 42);
}

// Test 2: Arithmetic operation (return 5 + 3)
TEST(InterpreterTest, SimpleAddition) {
  Graph g;

  // Build graph: Root -> Start -> ConI(5), ConI(3) -> AddI -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);
  Node* con5 = g.AddNode(2, Opcode::kConI);
  con5->set_prop("value", static_cast<int32_t>(5));

  Node* con3 = g.AddNode(3, Opcode::kConI);
  con3->set_prop("value", static_cast<int32_t>(3));

  Node* add = g.AddNode(4, Opcode::kAddI);
  add->set_input(0, con5);
  add->set_input(1, con3);

  Node* ret = g.AddNode(5, Opcode::kReturn);
  root->set_input(0, ret);   // Root points to Return
  ret->set_input(0, start);  // Control edge
  ret->set_input(1, add);    // Value edge

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 8);
}

// Test 3: Parameter access (return arg0 + arg1)
TEST(InterpreterTest, ParameterAddition) {
  Graph g;

  // Build graph: Root -> Start -> Parm(0), Parm(1) -> AddI -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* parm0 = g.AddNode(2, Opcode::kParm);
  parm0->set_prop("index", static_cast<int32_t>(0));
  parm0->set_input(0, start);

  Node* parm1 = g.AddNode(3, Opcode::kParm);
  parm1->set_prop("index", static_cast<int32_t>(1));
  parm1->set_input(0, start);

  Node* add = g.AddNode(4, Opcode::kAddI);
  add->set_input(0, parm0);
  add->set_input(1, parm1);

  Node* ret = g.AddNode(5, Opcode::kReturn);
  root->set_input(0, ret);   // Root points to Return
  ret->set_input(0, start);  // Control edge
  ret->set_input(1, add);    // Value edge

  // Execute with inputs [10, 20]
  Interpreter interp(g);
  std::vector<Value> inputs = {Value::MakeI32(10), Value::MakeI32(20)};
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 30);
}

// Test 4: Division by zero should throw
TEST(InterpreterTest, DivisionByZeroThrows) {
  Graph g;

  // Build graph: Root -> Start -> ConI(42) / ConI(0) -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);
  Node* con42 = g.AddNode(2, Opcode::kConI);
  con42->set_prop("value", static_cast<int32_t>(42));

  Node* con0 = g.AddNode(3, Opcode::kConI);
  con0->set_prop("value", static_cast<int32_t>(0));

  Node* div = g.AddNode(4, Opcode::kDivI);
  div->set_input(0, con42);
  div->set_input(1, con0);

  Node* ret = g.AddNode(5, Opcode::kReturn);
  root->set_input(0, ret);   // Root points to Return
  ret->set_input(0, start);  // Control edge
  ret->set_input(1, div);    // Value edge

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify - should be a Throw outcome
  EXPECT_EQ(outcome.kind, Outcome::Kind::kThrow);
  EXPECT_FALSE(outcome.exception_kind.empty());
}

// Test 5: Complex expression ((10 + 5) * 2)
TEST(InterpreterTest, ComplexExpression) {
  Graph g;

  // Build graph: Root -> Start -> (ConI(10) + ConI(5)) * ConI(2) -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);
  Node* con10 = g.AddNode(2, Opcode::kConI);
  con10->set_prop("value", static_cast<int32_t>(10));

  Node* con5 = g.AddNode(3, Opcode::kConI);
  con5->set_prop("value", static_cast<int32_t>(5));

  Node* add = g.AddNode(4, Opcode::kAddI);
  add->set_input(0, con10);
  add->set_input(1, con5);

  Node* con2 = g.AddNode(5, Opcode::kConI);
  con2->set_prop("value", static_cast<int32_t>(2));

  Node* mul = g.AddNode(6, Opcode::kMulI);
  mul->set_input(0, add);
  mul->set_input(1, con2);

  Node* ret = g.AddNode(7, Opcode::kReturn);
  root->set_input(0, ret);   // Root points to Return
  ret->set_input(0, start);  // Control edge
  ret->set_input(1, mul);    // Value edge

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 30);  // (10 + 5) * 2 = 30
}
