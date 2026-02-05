#include <gtest/gtest.h>

#include "suntv/ir/node.hpp"

using namespace sun;

TEST(NodeTest, BasicConstruction) {
  Node n(1, Opcode::kAddI);
  EXPECT_EQ(n.id(), 1);
  EXPECT_EQ(n.opcode(), Opcode::kAddI);
  EXPECT_EQ(n.num_inputs(), 0);
}

TEST(NodeTest, Properties) {
  Node n(1, Opcode::kConI);

  // Set and get properties
  n.set_prop("value", static_cast<int32_t>(42));
  EXPECT_TRUE(n.has_prop("value"));

  Property p = n.prop("value");
  EXPECT_EQ(std::get<int32_t>(p), 42);

  // Non-existent property
  EXPECT_FALSE(n.has_prop("nonexistent"));
  EXPECT_THROW(n.prop("nonexistent"), std::runtime_error);
}

TEST(NodeTest, Inputs) {
  Node n1(1, Opcode::kConI);
  Node n2(2, Opcode::kConI);
  Node n3(3, Opcode::kAddI);

  // Add inputs
  n3.AddInput(&n1);
  n3.AddInput(&n2);

  EXPECT_EQ(n3.num_inputs(), 2);
  EXPECT_EQ(n3.input(0), &n1);
  EXPECT_EQ(n3.input(1), &n2);

  // Out of range
  EXPECT_THROW(n3.input(2), std::out_of_range);
}

TEST(NodeTest, SetInput) {
  Node n1(1, Opcode::kConI);
  Node n2(2, Opcode::kConI);
  Node n3(3, Opcode::kAddI);

  // Set input at specific index (auto-resize)
  n3.set_input(0, &n1);
  n3.set_input(1, &n2);

  EXPECT_EQ(n3.num_inputs(), 2);
  EXPECT_EQ(n3.input(0), &n1);
  EXPECT_EQ(n3.input(1), &n2);

  // Replace input
  Node n4(4, Opcode::kConI);
  n3.set_input(0, &n4);
  EXPECT_EQ(n3.input(0), &n4);
}

TEST(NodeTest, TypeStamp) {
  Node n(1, Opcode::kAddI);

  // Default type is TOP
  EXPECT_EQ(n.type().GetKind(), TypeKind::TOP);

  // Set type
  n.set_type(TypeStamp(TypeKind::INT32));
  EXPECT_EQ(n.type().GetKind(), TypeKind::INT32);
  EXPECT_TRUE(n.type().IsInt32());
}

TEST(NodeTest, ToString) {
  Node n(42, Opcode::kAddI);
  std::string s = n.ToString();

  // Should contain opcode name and ID
  EXPECT_NE(s.find("AddI"), std::string::npos);
  EXPECT_NE(s.find("42"), std::string::npos);
}
