#include <gtest/gtest.h>

#include "suntv/ir/node.hpp"

using namespace sun;

TEST(NodeTest, BasicConstruction) {
  Node n(1, Opcode::AddI);
  EXPECT_EQ(n.id(), 1);
  EXPECT_EQ(n.opcode(), Opcode::AddI);
}
