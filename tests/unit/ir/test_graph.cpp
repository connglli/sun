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

  Node* n1 = g.add_node(1, Opcode::Start);
  Node* n2 = g.add_node(2, Opcode::ConI);
  Node* n3 = g.add_node(3, Opcode::Return);

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

  Node* n1 = g.add_node(1, Opcode::Start);
  Node* n2 = g.add_node(10, Opcode::ConI);

  EXPECT_EQ(g.node(1), n1);
  EXPECT_EQ(g.node(10), n2);
  EXPECT_EQ(g.node(99), nullptr);  // Non-existent
}

TEST(GraphTest, SpecialNodes) {
  Graph g;

  // Add Start node
  Node* start = g.add_node(1, Opcode::Start);
  EXPECT_EQ(g.start(), start);

  // Add Root node
  Node* root = g.add_node(0, Opcode::Root);
  EXPECT_EQ(g.root(), root);
}

TEST(GraphTest, ParameterNodes) {
  Graph g;

  g.add_node(1, Opcode::Start);
  Node* p1 = g.add_node(10, Opcode::Parm);
  Node* p2 = g.add_node(11, Opcode::Parm);
  g.add_node(20, Opcode::AddI);

  auto params = g.parameter_nodes();
  EXPECT_EQ(params.size(), 2);

  // Should contain both parameter nodes
  EXPECT_TRUE(std::find(params.begin(), params.end(), p1) != params.end());
  EXPECT_TRUE(std::find(params.begin(), params.end(), p2) != params.end());
}

TEST(GraphTest, ControlNodes) {
  Graph g;

  Node* start = g.add_node(1, Opcode::Start);
  g.add_node(10, Opcode::ConI);  // Not control
  Node* if_node = g.add_node(20, Opcode::If);
  Node* ret = g.add_node(30, Opcode::Return);

  auto controls = g.control_nodes();
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
  Node* start = g.add_node(0, Opcode::Start);
  Node* p0 = g.add_node(1, Opcode::Parm);
  Node* p1 = g.add_node(2, Opcode::Parm);
  Node* add = g.add_node(3, Opcode::AddI);
  Node* ret = g.add_node(4, Opcode::Return);

  // Connect edges
  p0->add_input(start);
  p1->add_input(start);
  add->add_input(p0);
  add->add_input(p1);
  ret->add_input(start);  // Control
  ret->add_input(add);    // Value

  // Verify structure
  EXPECT_EQ(add->num_inputs(), 2);
  EXPECT_EQ(add->input(0), p0);
  EXPECT_EQ(add->input(1), p1);

  EXPECT_EQ(ret->num_inputs(), 2);
  EXPECT_EQ(ret->input(0), start);
  EXPECT_EQ(ret->input(1), add);
}
