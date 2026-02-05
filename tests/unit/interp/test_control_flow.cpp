#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

// Test 1: Bool node (convert comparison to boolean)
// Graph: return (5 > 3) ? 1 : 0
// Simplified: CmpI(5, 3) -> Bool -> return as i32
TEST(ControlFlowTest, BoolNodeFromComparison) {
  Graph g;

  // Build graph: Root -> Start -> ConI(5), ConI(3) -> CmpI -> Bool -> Return
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* con5 = g.AddNode(2, Opcode::kConI);
  con5->set_prop("value", static_cast<int32_t>(5));

  Node* con3 = g.AddNode(3, Opcode::kConI);
  con3->set_prop("value", static_cast<int32_t>(3));

  Node* cmp = g.AddNode(4, Opcode::kCmpI);
  cmp->set_input(0, con5);
  cmp->set_input(1, con3);

  // Bool node takes comparison result and mask
  Node* bool_node = g.AddNode(5, Opcode::kBool);
  bool_node->set_input(0, cmp);
  bool_node->set_prop("mask",
                      static_cast<int32_t>(4));  // GT mask (greater than)

  Node* ret = g.AddNode(6, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, bool_node);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify: 5 > 3 is true
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kBool);
  EXPECT_TRUE(outcome.return_value->as_bool());
}

// Test 2: Simple If-then-else (return arg0 > 10 ? 1 : 0)
TEST(ControlFlowTest, SimpleIfThenElse) {
  Graph g;

  // Build graph:
  // Start -> Parm(0) -> CmpI(parm, 10) -> Bool -> If
  //                                               |
  //                                        +------+------+
  //                                        |             |
  //                                     IfTrue        IfFalse
  //                                        |             |
  //                                     ConI(1)       ConI(0)
  //                                        |             |
  //                                        +------+------+
  //                                               |
  //                                            Region
  //                                               |
  //                                              Phi
  //                                               |
  //                                            Return

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* parm = g.AddNode(2, Opcode::kParm);
  parm->set_prop("index", static_cast<int32_t>(0));
  parm->set_input(0, start);

  Node* con10 = g.AddNode(3, Opcode::kConI);
  con10->set_prop("value", static_cast<int32_t>(10));

  Node* cmp = g.AddNode(4, Opcode::kCmpI);
  cmp->set_input(0, parm);
  cmp->set_input(1, con10);

  Node* bool_node = g.AddNode(5, Opcode::kBool);
  bool_node->set_input(0, cmp);
  bool_node->set_prop("mask", static_cast<int32_t>(4));  // GT mask

  Node* if_node = g.AddNode(6, Opcode::kIf);
  if_node->set_input(0, start);      // Control
  if_node->set_input(1, bool_node);  // Condition

  Node* if_true = g.AddNode(7, Opcode::kIfTrue);
  if_true->set_input(0, if_node);

  Node* if_false = g.AddNode(8, Opcode::kIfFalse);
  if_false->set_input(0, if_node);

  Node* con1 = g.AddNode(9, Opcode::kConI);
  con1->set_prop("value", static_cast<int32_t>(1));

  Node* con0 = g.AddNode(10, Opcode::kConI);
  con0->set_prop("value", static_cast<int32_t>(0));

  Node* region = g.AddNode(11, Opcode::kRegion);
  region->set_input(0, if_true);
  region->set_input(1, if_false);

  Node* phi = g.AddNode(12, Opcode::kPhi);
  phi->set_input(0, region);  // Control
  phi->set_input(1, con1);    // Value from true path
  phi->set_input(2, con0);    // Value from false path

  Node* ret = g.AddNode(13, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, region);
  ret->set_input(1, phi);

  // Execute with arg0 = 15 (> 10, should return 1)
  Interpreter interp(g);
  std::vector<Value> inputs = {Value::MakeI32(15)};
  Outcome outcome = interp.Execute(inputs);

  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 1);
}

// Test 3: If-then-else with arg0 <= 10 (should return 0)
TEST(ControlFlowTest, IfThenElseFalsePath) {
  Graph g;

  // Same structure as Test 2
  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* parm = g.AddNode(2, Opcode::kParm);
  parm->set_prop("index", static_cast<int32_t>(0));
  parm->set_input(0, start);

  Node* con10 = g.AddNode(3, Opcode::kConI);
  con10->set_prop("value", static_cast<int32_t>(10));

  Node* cmp = g.AddNode(4, Opcode::kCmpI);
  cmp->set_input(0, parm);
  cmp->set_input(1, con10);

  Node* bool_node = g.AddNode(5, Opcode::kBool);
  bool_node->set_input(0, cmp);
  bool_node->set_prop("mask", static_cast<int32_t>(4));  // GT mask

  Node* if_node = g.AddNode(6, Opcode::kIf);
  if_node->set_input(0, start);
  if_node->set_input(1, bool_node);

  Node* if_true = g.AddNode(7, Opcode::kIfTrue);
  if_true->set_input(0, if_node);

  Node* if_false = g.AddNode(8, Opcode::kIfFalse);
  if_false->set_input(0, if_node);

  Node* con1 = g.AddNode(9, Opcode::kConI);
  con1->set_prop("value", static_cast<int32_t>(1));

  Node* con0 = g.AddNode(10, Opcode::kConI);
  con0->set_prop("value", static_cast<int32_t>(0));

  Node* region = g.AddNode(11, Opcode::kRegion);
  region->set_input(0, if_true);
  region->set_input(1, if_false);

  Node* phi = g.AddNode(12, Opcode::kPhi);
  phi->set_input(0, region);
  phi->set_input(1, con1);
  phi->set_input(2, con0);

  Node* ret = g.AddNode(13, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, region);
  ret->set_input(1, phi);

  // Execute with arg0 = 5 (<= 10, should return 0)
  Interpreter interp(g);
  std::vector<Value> inputs = {Value::MakeI32(5)};
  Outcome outcome = interp.Execute(inputs);

  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 0);
}

// Test 4: Nested if (return arg0 > 10 ? (arg0 > 20 ? 2 : 1) : 0)
TEST(ControlFlowTest, NestedIf) {
  Graph g;

  // Build graph with nested if:
  // if (arg0 > 10) {
  //   if (arg0 > 20) return 2; else return 1;
  // } else {
  //   return 0;
  // }

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* parm = g.AddNode(2, Opcode::kParm);
  parm->set_prop("index", static_cast<int32_t>(0));
  parm->set_input(0, start);

  // Outer if: arg0 > 10
  Node* con10 = g.AddNode(3, Opcode::kConI);
  con10->set_prop("value", static_cast<int32_t>(10));

  Node* cmp1 = g.AddNode(4, Opcode::kCmpI);
  cmp1->set_input(0, parm);
  cmp1->set_input(1, con10);

  Node* bool1 = g.AddNode(5, Opcode::kBool);
  bool1->set_input(0, cmp1);
  bool1->set_prop("mask", static_cast<int32_t>(4));  // GT

  Node* if1 = g.AddNode(6, Opcode::kIf);
  if1->set_input(0, start);
  if1->set_input(1, bool1);

  Node* if1_true = g.AddNode(7, Opcode::kIfTrue);
  if1_true->set_input(0, if1);

  Node* if1_false = g.AddNode(8, Opcode::kIfFalse);
  if1_false->set_input(0, if1);

  // Inner if (on true path): arg0 > 20
  Node* con20 = g.AddNode(9, Opcode::kConI);
  con20->set_prop("value", static_cast<int32_t>(20));

  Node* cmp2 = g.AddNode(10, Opcode::kCmpI);
  cmp2->set_input(0, parm);
  cmp2->set_input(1, con20);

  Node* bool2 = g.AddNode(11, Opcode::kBool);
  bool2->set_input(0, cmp2);
  bool2->set_prop("mask", static_cast<int32_t>(4));  // GT

  Node* if2 = g.AddNode(12, Opcode::kIf);
  if2->set_input(0, if1_true);
  if2->set_input(1, bool2);

  Node* if2_true = g.AddNode(13, Opcode::kIfTrue);
  if2_true->set_input(0, if2);

  Node* if2_false = g.AddNode(14, Opcode::kIfFalse);
  if2_false->set_input(0, if2);

  // Constants
  Node* con2 = g.AddNode(15, Opcode::kConI);
  con2->set_prop("value", static_cast<int32_t>(2));

  Node* con1 = g.AddNode(16, Opcode::kConI);
  con1->set_prop("value", static_cast<int32_t>(1));

  Node* con0 = g.AddNode(17, Opcode::kConI);
  con0->set_prop("value", static_cast<int32_t>(0));

  // Inner region (merge if2_true and if2_false)
  Node* region2 = g.AddNode(18, Opcode::kRegion);
  region2->set_input(0, if2_true);
  region2->set_input(1, if2_false);

  Node* phi2 = g.AddNode(19, Opcode::kPhi);
  phi2->set_input(0, region2);
  phi2->set_input(1, con2);  // if2_true
  phi2->set_input(2, con1);  // if2_false

  // Outer region (merge region2 and if1_false)
  Node* region1 = g.AddNode(20, Opcode::kRegion);
  region1->set_input(0, region2);
  region1->set_input(1, if1_false);

  Node* phi1 = g.AddNode(21, Opcode::kPhi);
  phi1->set_input(0, region1);
  phi1->set_input(1, phi2);  // From true path
  phi1->set_input(2, con0);  // From false path

  Node* ret = g.AddNode(22, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, region1);
  ret->set_input(1, phi1);

  // Test with arg0 = 25 (> 20, should return 2)
  Interpreter interp(g);
  std::vector<Value> inputs = {Value::MakeI32(25)};
  Outcome outcome = interp.Execute(inputs);

  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 2);
}

// Test 5: CMoveI (conditional move without control flow)
// return arg0 > 10 ? 100 : 200
TEST(ControlFlowTest, ConditionalMove) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* parm = g.AddNode(2, Opcode::kParm);
  parm->set_prop("index", static_cast<int32_t>(0));
  parm->set_input(0, start);

  Node* con10 = g.AddNode(3, Opcode::kConI);
  con10->set_prop("value", static_cast<int32_t>(10));

  Node* cmp = g.AddNode(4, Opcode::kCmpI);
  cmp->set_input(0, parm);
  cmp->set_input(1, con10);

  Node* bool_node = g.AddNode(5, Opcode::kBool);
  bool_node->set_input(0, cmp);
  bool_node->set_prop("mask", static_cast<int32_t>(4));  // GT

  Node* con100 = g.AddNode(6, Opcode::kConI);
  con100->set_prop("value", static_cast<int32_t>(100));

  Node* con200 = g.AddNode(7, Opcode::kConI);
  con200->set_prop("value", static_cast<int32_t>(200));

  // CMoveI: input(0) = condition, input(1) = true_val, input(2) = false_val
  Node* cmove = g.AddNode(8, Opcode::kCMoveI);
  cmove->set_input(0, bool_node);
  cmove->set_input(1, con100);
  cmove->set_input(2, con200);

  Node* ret = g.AddNode(9, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, cmove);

  // Test with arg0 = 15 (> 10, should return 100)
  Interpreter interp(g);
  std::vector<Value> inputs = {Value::MakeI32(15)};
  Outcome outcome = interp.Execute(inputs);

  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 100);
}
