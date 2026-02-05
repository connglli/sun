#include <gtest/gtest.h>

#include "suntv/interp/value.hpp"

using namespace sun;

TEST(ValueTest, MakeInt32) {
  Value v = Value::MakeI32(42);
  EXPECT_EQ(v.kind, Value::Kind::kI32);
  EXPECT_EQ(v.data.i32, 42);
  EXPECT_TRUE(v.is_i32());
  EXPECT_EQ(v.as_i32(), 42);
}

TEST(ValueTest, MakeInt64) {
  Value v = Value::MakeI64(123456789012345LL);
  EXPECT_EQ(v.kind, Value::Kind::kI64);
  EXPECT_TRUE(v.is_i64());
  EXPECT_EQ(v.as_i64(), 123456789012345LL);
}

TEST(ValueTest, MakeBool) {
  Value v_true = Value::MakeBool(true);
  Value v_false = Value::MakeBool(false);

  EXPECT_TRUE(v_true.is_bool());
  EXPECT_TRUE(v_true.as_bool());
  EXPECT_FALSE(v_false.as_bool());
}

TEST(ValueTest, MakeRef) {
  Value v = Value::MakeRef(7);
  EXPECT_TRUE(v.is_ref());
  EXPECT_EQ(v.as_ref(), 7);
}

TEST(ValueTest, MakeNull) {
  Value v = Value::MakeNull();
  EXPECT_TRUE(v.is_null());
  EXPECT_EQ(v.as_ref(), 0);
}

TEST(ValueTest, ToString) {
  EXPECT_EQ(Value::MakeI32(42).ToString(), "i32:42");
  EXPECT_EQ(Value::MakeI64(999).ToString(), "i64:999");
  EXPECT_EQ(Value::MakeBool(true).ToString(), "bool:true");
  EXPECT_EQ(Value::MakeRef(5).ToString(), "ref:5");
  EXPECT_EQ(Value::MakeNull().ToString(), "null");
}

TEST(ValueTest, TypeCheckErrors) {
  Value v = Value::MakeI32(42);
  EXPECT_THROW(v.as_i64(), std::runtime_error);
  EXPECT_THROW(v.as_bool(), std::runtime_error);
  EXPECT_THROW(v.as_ref(), std::runtime_error);
}
