#include <gtest/gtest.h>

#include "suntv/interp/heap.hpp"

using namespace sun;

TEST(HeapTest, AllocateObject) {
  ConcreteHeap heap;
  Ref r1 = heap.AllocateObject();
  Ref r2 = heap.AllocateObject();

  EXPECT_EQ(r1, 1);
  EXPECT_EQ(r2, 2);
  EXPECT_NE(r1, r2);
}

TEST(HeapTest, AllocateArray) {
  ConcreteHeap heap;
  Ref arr = heap.AllocateArray(5);

  EXPECT_EQ(arr, 1);
  EXPECT_EQ(heap.ArrayLength(arr), 5);
}

TEST(HeapTest, FieldReadWrite) {
  ConcreteHeap heap;
  Ref obj = heap.AllocateObject();

  // Write and read field
  heap.WriteField(obj, "x", Value::MakeI32(42));
  Value v = heap.ReadField(obj, "x");

  EXPECT_TRUE(v.is_i32());
  EXPECT_EQ(v.as_i32(), 42);

  // Read uninitialized field (should return default 0)
  Value v2 = heap.ReadField(obj, "uninitialized");
  EXPECT_TRUE(v2.is_i32());
  EXPECT_EQ(v2.as_i32(), 0);
}

TEST(HeapTest, ArrayReadWrite) {
  ConcreteHeap heap;
  Ref arr = heap.AllocateArray(3);

  // Write and read array elements
  heap.WriteArray(arr, 0, Value::MakeI32(10));
  heap.WriteArray(arr, 1, Value::MakeI32(20));
  heap.WriteArray(arr, 2, Value::MakeI32(30));

  EXPECT_EQ(heap.ReadArray(arr, 0).as_i32(), 10);
  EXPECT_EQ(heap.ReadArray(arr, 1).as_i32(), 20);
  EXPECT_EQ(heap.ReadArray(arr, 2).as_i32(), 30);
}

TEST(HeapTest, ArrayBoundsChecking) {
  ConcreteHeap heap;
  Ref arr = heap.AllocateArray(2);

  EXPECT_THROW(heap.ReadArray(arr, -1), std::runtime_error);
  EXPECT_THROW(heap.ReadArray(arr, 2), std::runtime_error);
  EXPECT_THROW(heap.WriteArray(arr, 3, Value::MakeI32(0)), std::runtime_error);
}

TEST(HeapTest, NegativeArrayLength) {
  ConcreteHeap heap;
  EXPECT_THROW(heap.AllocateArray(-1), std::runtime_error);
}

TEST(HeapTest, Dump) {
  ConcreteHeap heap;
  Ref obj = heap.AllocateObject();
  heap.WriteField(obj, "value", Value::MakeI32(99));

  std::string dump = heap.Dump();
  EXPECT_NE(dump.find("ref:1.value"), std::string::npos);
  EXPECT_NE(dump.find("i32:99"), std::string::npos);
}
