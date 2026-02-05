#include <gtest/gtest.h>

#include "suntv/interp/interpreter.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;

// Test 1: Simple object allocation
// allocate object -> return ref
TEST(MemoryTest, SimpleAllocate) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Allocate node: input(0) = control
  Node* alloc = g.AddNode(2, Opcode::kAllocate);
  alloc->set_input(0, start);
  alloc->set_prop("type", std::string("Object"));

  Node* ret = g.AddNode(3, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, alloc);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify: returns a valid reference
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kRef);
  EXPECT_GE(outcome.return_value->as_ref(), 1);  // Valid ref (>= 1)
}

// Test 2: Array allocation with length
// allocate array[10] -> return ref
TEST(MemoryTest, AllocateArray) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  Node* len = g.AddNode(2, Opcode::kConI);
  len->set_prop("value", static_cast<int32_t>(10));

  // AllocateArray: input(0) = control, input(1) = length
  Node* alloc = g.AddNode(3, Opcode::kAllocateArray);
  alloc->set_input(0, start);
  alloc->set_input(1, len);
  alloc->set_prop("elem_type", std::string("int"));

  Node* ret = g.AddNode(4, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, alloc);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kRef);

  // Check that heap has array with length 10
  Ref arr_ref = outcome.return_value->as_ref();
  EXPECT_EQ(outcome.heap.ArrayLength(arr_ref), 10);
}

// Test 3: Store and load field
// obj = allocate; obj.field = 42; return obj.field
TEST(MemoryTest, StoreAndLoadField) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Allocate object
  Node* alloc = g.AddNode(2, Opcode::kAllocate);
  alloc->set_input(0, start);

  // Constant value to store
  Node* val42 = g.AddNode(3, Opcode::kConI);
  val42->set_prop("value", static_cast<int32_t>(42));

  // Store: input(0) = control, input(1) = memory, input(2) = base, input(3) =
  // value
  Node* store = g.AddNode(4, Opcode::kStoreI);
  store->set_input(0, start);  // Control
  store->set_input(1,
                   start);  // Memory (simplified - use Start as initial memory)
  store->set_input(2, alloc);  // Base object
  store->set_input(3, val42);  // Value
  store->set_prop("field", std::string("x"));

  // Load: input(0) = control, input(1) = memory, input(2) = base
  Node* load = g.AddNode(5, Opcode::kLoadI);
  load->set_input(0, start);  // Control
  load->set_input(1, store);  // Memory (after store)
  load->set_input(2, alloc);  // Base object
  load->set_prop("field", std::string("x"));

  Node* ret = g.AddNode(6, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, load);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify: loaded value should be 42
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 42);
}

// Test 4: Array store and load
// arr = allocate[5]; arr[2] = 99; return arr[2]
TEST(MemoryTest, ArrayStoreAndLoad) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // Allocate array[5]
  Node* len = g.AddNode(2, Opcode::kConI);
  len->set_prop("value", static_cast<int32_t>(5));

  Node* alloc = g.AddNode(3, Opcode::kAllocateArray);
  alloc->set_input(0, start);
  alloc->set_input(1, len);

  // Index and value
  Node* idx = g.AddNode(4, Opcode::kConI);
  idx->set_prop("value", static_cast<int32_t>(2));

  Node* val99 = g.AddNode(5, Opcode::kConI);
  val99->set_prop("value", static_cast<int32_t>(99));

  // StoreI to array: input(0) = control, input(1) = memory,
  //                  input(2) = base, input(3) = index, input(4) = value
  Node* store = g.AddNode(6, Opcode::kStoreI);
  store->set_input(0, start);
  store->set_input(1, start);      // Memory
  store->set_input(2, alloc);      // Array base
  store->set_input(3, idx);        // Index
  store->set_input(4, val99);      // Value
  store->set_prop("array", true);  // Mark as array access

  // LoadI from array
  Node* load = g.AddNode(7, Opcode::kLoadI);
  load->set_input(0, start);
  load->set_input(1, store);  // Memory after store
  load->set_input(2, alloc);  // Array base
  load->set_input(3, idx);    // Index
  load->set_prop("array", true);

  Node* ret = g.AddNode(8, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, load);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kI32);
  EXPECT_EQ(outcome.return_value->as_i32(), 99);
}

// Test 5: Multiple allocations (unique refs)
// obj1 = allocate; obj2 = allocate; return obj1 != obj2
TEST(MemoryTest, MultipleAllocations) {
  Graph g;

  Node* root = g.AddNode(0, Opcode::kRoot);
  Node* start = g.AddNode(1, Opcode::kStart);

  // First allocation
  Node* alloc1 = g.AddNode(2, Opcode::kAllocate);
  alloc1->set_input(0, start);

  // Second allocation
  Node* alloc2 = g.AddNode(3, Opcode::kAllocate);
  alloc2->set_input(0, start);

  // Compare refs (should be different)
  Node* cmp = g.AddNode(4, Opcode::kCmpP);
  cmp->set_input(0, alloc1);
  cmp->set_input(1, alloc2);

  // Convert to bool (NE = not equal, mask = 5 = LT|GT)
  Node* bool_node = g.AddNode(5, Opcode::kBool);
  bool_node->set_input(0, cmp);
  bool_node->set_prop("mask", static_cast<int32_t>(5));  // NE mask

  Node* ret = g.AddNode(6, Opcode::kReturn);
  root->set_input(0, ret);
  ret->set_input(0, start);
  ret->set_input(1, bool_node);

  // Execute
  Interpreter interp(g);
  std::vector<Value> inputs;
  Outcome outcome = interp.Execute(inputs);

  // Verify: refs should be different
  EXPECT_EQ(outcome.kind, Outcome::Kind::kReturn);
  EXPECT_TRUE(outcome.return_value.has_value());
  EXPECT_EQ(outcome.return_value->kind, Value::Kind::kBool);
  EXPECT_TRUE(outcome.return_value->as_bool());  // obj1 != obj2
}
