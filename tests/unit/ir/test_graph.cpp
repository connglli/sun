#include <gtest/gtest.h>

#include "suntv/ir/graph.hpp"

using namespace sun;

TEST(GraphTest, BasicConstruction) {
  Graph g;
  EXPECT_EQ(g.GetStart(), nullptr);
  EXPECT_EQ(g.GetRoot(), nullptr);
  EXPECT_EQ(g.GetNodes().size(), 0);
}

TEST(GraphTest, AddNodes) {
  Graph g;

  Node* n1 = g.AddNode(1, Opcode::Start);
  Node* n2 = g.AddNode(2, Opcode::ConI);
  Node* n3 = g.AddNode(3, Opcode::Return);

  EXPECT_NE(n1, nullptr);
  EXPECT_NE(n2, nullptr);
  EXPECT_NE(n3, nullptr);

  EXPECT_EQ(n1->Id(), 1);
  EXPECT_EQ(n2->Id(), 2);
  EXPECT_EQ(n3->Id(), 3);

  EXPECT_EQ(g.GetNodes().size(), 3);
}

TEST(GraphTest, NodeLookup) {
  Graph g;

  Node* n1 = g.AddNode(1, Opcode::Start);
  Node* n2 = g.AddNode(10, Opcode::ConI);

  EXPECT_EQ(g.GetNode(1), n1);
  EXPECT_EQ(g.GetNode(10), n2);
  EXPECT_EQ(g.GetNode(99), nullptr);  // Non-existent
}

TEST(GraphTest, SpecialNodes) {
  Graph g;

  // Add Start node
  Node* start = g.AddNode(1, Opcode::Start);
  EXPECT_EQ(g.GetStart(), start);

  // Add Root node
  Node* root = g.AddNode(0, Opcode::Root);
  EXPECT_EQ(g.GetRoot(), root);
}

TEST(GraphTest, ParameterNodes) {
  Graph g;

  g.AddNode(1, Opcode::Start);
  Node* p1 = g.AddNode(10, Opcode::Parm);
  Node* p2 = g.AddNode(11, Opcode::Parm);
  g.AddNode(20, Opcode::AddI);

  auto params = g.GetParameterNodes();
  EXPECT_EQ(params.size(), 2);

  // Should contain both parameter nodes
  EXPECT_TRUE(std::find(params.begin(), params.end(), p1) != params.end());
  EXPECT_TRUE(std::find(params.begin(), params.end(), p2) != params.end());
}

TEST(GraphTest, ControlNodes) {
  Graph g;

  Node* start = g.AddNode(1, Opcode::Start);
  g.AddNode(10, Opcode::ConI);  // Not control
  Node* if_node = g.AddNode(20, Opcode::If);
  Node* ret = g.AddNode(30, Opcode::Return);

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
  Node* start = g.AddNode(0, Opcode::Start);
  Node* p0 = g.AddNode(1, Opcode::Parm);
  Node* p1 = g.AddNode(2, Opcode::Parm);
  Node* add = g.AddNode(3, Opcode::AddI);
  Node* ret = g.AddNode(4, Opcode::Return);

  // Connect edges
  p0->AddInput(start);
  p1->AddInput(start);
  add->AddInput(p0);
  add->AddInput(p1);
  ret->AddInput(start);  // Control
  ret->AddInput(add);    // Value

  // Verify structure
  EXPECT_EQ(add->NumInputs(), 2);
  EXPECT_EQ(add->GetInput(0), p0);
  EXPECT_EQ(add->GetInput(1), p1);

  EXPECT_EQ(ret->NumInputs(), 2);
  EXPECT_EQ(ret->GetInput(0), start);
  EXPECT_EQ(ret->GetInput(1), add);
}
