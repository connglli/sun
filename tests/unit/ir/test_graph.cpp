#include <gtest/gtest.h>

#include "suntv/ir/graph.hpp"

using namespace sun;

TEST(GraphTest, BasicConstruction) {
  Graph g;
  EXPECT_NE(&g, nullptr);
}
