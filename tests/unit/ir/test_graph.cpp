#include <gtest/gtest.h>

#include "suntv/ir/graph.hpp"

using namespace sun;

TEST(GraphTest, BasicConstruction) {
  Graph g;
  EXPECT_EQ(g.start(), nullptr);
  EXPECT_EQ(g.root(), nullptr);
  EXPECT_EQ(g.nodes().size(), 0);
}

TEST(GraphTest, AddNodes) {
  Graph g;

  Node* n1 = g.AddNode(1, Opcode::kStart);
  Node* n2 = g.AddNode(2, Opcode::kConI);
  Node* n3 = g.AddNode(3, Opcode::kReturn);

  EXPECT_NE(n1, nullptr);
  EXPECT_NE(n2, nullptr);
  EXPECT_NE(n3, nullptr);

  EXPECT_EQ(n1->id(), 1);
  EXPECT_EQ(n2->id(), 2);
  EXPECT_EQ(n3->id(), 3);

  EXPECT_EQ(g.nodes().size(), 3);
}

TEST(GraphTest, NodeLookup) {
  Graph g;

  Node* n1 = g.AddNode(1, Opcode::kStart);
  Node* n2 = g.AddNode(10, Opcode::kConI);

  EXPECT_EQ(g.node(1), n1);
  EXPECT_EQ(g.node(10), n2);
  EXPECT_EQ(g.node(99), nullptr);  // Non-existent
}

TEST(GraphTest, SpecialNodes) {
  Graph g;

  // Add Start node
  Node* start = g.AddNode(1, Opcode::kStart);
  EXPECT_EQ(g.start(), start);

  // Add Root node
  Node* root = g.AddNode(0, Opcode::kRoot);
  EXPECT_EQ(g.root(), root);
}

TEST(GraphTest, ParameterNodes) {
  Graph g;

  g.AddNode(1, Opcode::kStart);
  Node* p1 = g.AddNode(10, Opcode::kParm);
  Node* p2 = g.AddNode(11, Opcode::kParm);
  g.AddNode(20, Opcode::kAddI);

  auto params = g.GetParameterNodes();
  EXPECT_EQ(params.size(), 2);

  // Should contain both parameter nodes
  EXPECT_TRUE(std::find(params.begin(), params.end(), p1) != params.end());
  EXPECT_TRUE(std::find(params.begin(), params.end(), p2) != params.end());
}

TEST(GraphTest, ControlNodes) {
  Graph g;

  Node* start = g.AddNode(1, Opcode::kStart);
  g.AddNode(10, Opcode::kConI);  // Not control
  Node* if_node = g.AddNode(20, Opcode::kIf);
  Node* ret = g.AddNode(30, Opcode::kReturn);

  auto controls = g.GetControlNodes();
  EXPECT_EQ(controls.size(), 3);

  // Should contain all control nodes
  EXPECT_TRUE(std::find(controls.begin(), controls.end(), start) !=
              controls.end());
  EXPECT_TRUE(std::find(controls.begin(), controls.end(), if_node) !=
              controls.end());
  EXPECT_TRUE(std::find(controls.begin(), controls.end(), ret) !=
              controls.end());
}

TEST(GraphTest, BuildSimpleGraph) {
  Graph g;

  // Build: Start -> Parm(0) -> Parm(1) -> AddI -> Return
  Node* start = g.AddNode(0, Opcode::kStart);
  Node* p0 = g.AddNode(1, Opcode::kParm);
  Node* p1 = g.AddNode(2, Opcode::kParm);
  Node* add = g.AddNode(3, Opcode::kAddI);
  Node* ret = g.AddNode(4, Opcode::kReturn);

  // Connect edges
  p0->AddInput(start);
  p1->AddInput(start);
  add->AddInput(p0);
  add->AddInput(p1);
  ret->AddInput(start);  // Control
  ret->AddInput(add);    // Value

  // Verify structure
  EXPECT_EQ(add->num_inputs(), 2);
  EXPECT_EQ(add->input(0), p0);
  EXPECT_EQ(add->input(1), p1);

  EXPECT_EQ(ret->num_inputs(), 2);
  EXPECT_EQ(ret->input(0), start);
  EXPECT_EQ(ret->input(1), add);
}
