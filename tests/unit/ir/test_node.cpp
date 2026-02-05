#include <gtest/gtest.h>

#include "suntv/ir/node.hpp"

using namespace sun;

TEST(NodeTest, BasicConstruction) {
  Node n(1, Opcode::AddI);
  EXPECT_EQ(n.Id(), 1);
  EXPECT_EQ(n.GetOpcode(), Opcode::AddI);
  EXPECT_EQ(n.NumInputs(), 0);
}

TEST(NodeTest, Properties) {
  Node n(1, Opcode::ConI);

  // Set and get properties
  n.SetProp("value", static_cast<int32_t>(42));
  EXPECT_TRUE(n.HasProp("value"));

  Property p = n.GetProp("value");
  EXPECT_EQ(std::get<int32_t>(p), 42);

  // Non-existent property
  EXPECT_FALSE(n.HasProp("nonexistent"));
  EXPECT_THROW(n.GetProp("nonexistent"), std::runtime_error);
}

TEST(NodeTest, Inputs) {
  Node n1(1, Opcode::ConI);
  Node n2(2, Opcode::ConI);
  Node n3(3, Opcode::AddI);

  // Add inputs
  n3.AddInput(&n1);
  n3.AddInput(&n2);

  EXPECT_EQ(n3.NumInputs(), 2);
  EXPECT_EQ(n3.GetInput(0), &n1);
  EXPECT_EQ(n3.GetInput(1), &n2);

  // Out of range
  EXPECT_THROW(n3.GetInput(2), std::out_of_range);
}

TEST(NodeTest, SetInput) {
  Node n1(1, Opcode::ConI);
  Node n2(2, Opcode::ConI);
  Node n3(3, Opcode::AddI);

  // Set input at specific index (auto-resize)
  n3.SetInput(0, &n1);
  n3.SetInput(1, &n2);

  EXPECT_EQ(n3.NumInputs(), 2);
  EXPECT_EQ(n3.GetInput(0), &n1);
  EXPECT_EQ(n3.GetInput(1), &n2);

  // Replace input
  Node n4(4, Opcode::ConI);
  n3.SetInput(0, &n4);
  EXPECT_EQ(n3.GetInput(0), &n4);
}

TEST(NodeTest, TypeStamp) {
  Node n(1, Opcode::AddI);

  // Default type is TOP
  EXPECT_EQ(n.GetType().GetKind(), TypeKind::TOP);

  // Set type
  n.SetType(TypeStamp(TypeKind::INT32));
  EXPECT_EQ(n.GetType().GetKind(), TypeKind::INT32);
  EXPECT_TRUE(n.GetType().IsInt32());
}

TEST(NodeTest, ToString) {
  Node n(42, Opcode::AddI);
  std::string s = n.ToString();

  // Should contain opcode name and ID
  EXPECT_NE(s.find("AddI"), std::string::npos);
  EXPECT_NE(s.find("42"), std::string::npos);
}
