#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

// Proj nodes are common in real C2 IGV graphs.
// For the concrete interpreter prototype, we treat Proj as pass-through of its
// first value input (or dummy 0 if none).

TEST(InterpreterProjTest, ProjValuePassThrough) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* con = g.AddNode(2, Opcode::kConI);
  con->set_prop("value", static_cast<int32_t>(42));

  Node* proj = g.AddNode(3, Opcode::kProj);
  proj->set_input(0, start);
  proj->set_input(1, con);

  // Sanity: Proj should expose its projected value as a value input.
  ASSERT_EQ(proj->schema(), NodeSchema::kS8_Projection);
  auto proj_vals = proj->value_inputs();
  ASSERT_EQ(proj_vals.size(), 1u);
  EXPECT_EQ(proj_vals[0], con);

  Node* ret = g.AddNode(4, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, proj);

  Interpreter interp(g);
  Outcome outcome = interp.Execute({});
  ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
  ASSERT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->as_i32(), 42);
}

TEST(InterpreterProjTest, ProjWithNoValueInputsReturnsZero) {
  Graph g;
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* proj = g.AddNode(2, Opcode::kProj);
  proj->set_input(0, start);

  Node* ret = g.AddNode(3, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, proj);

  Interpreter interp(g);
  Outcome outcome = interp.Execute({});
  ASSERT_EQ(outcome.kind, Outcome::Kind::kReturn);
  ASSERT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->as_i32(), 0);
}
