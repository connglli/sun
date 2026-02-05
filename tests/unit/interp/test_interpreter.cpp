#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

TEST(InterpreterTest, Stub) {
  Graph g;
  Interpreter interp(g);
  EXPECT_NE(&interp, nullptr);
}
