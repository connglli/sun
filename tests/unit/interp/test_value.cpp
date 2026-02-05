#include <gtest/gtest.h>

#include "suntv/interp/value.hpp"

using namespace sun;

TEST(ValueTest, MakeInt32) {
  Value v = Value::make_i32(42);
  EXPECT_EQ(v.kind, Value::Kind::I32);
  EXPECT_EQ(v.data.i32, 42);
}
