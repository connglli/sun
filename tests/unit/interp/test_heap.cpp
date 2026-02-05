#include <gtest/gtest.h>

#include "suntv/interp/heap.hpp"

using namespace sun;

TEST(HeapTest, Stub) {
  ConcreteHeap heap;
  EXPECT_NE(&heap, nullptr);
}
