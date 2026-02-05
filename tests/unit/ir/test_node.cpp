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
  EXPECT_EQ(n.type().kind(), TypeKind::kTop);

  // Set type
  n.set_type(TypeStamp(TypeKind::kInt32));
  EXPECT_EQ(n.type().kind(), TypeKind::kInt32);
  EXPECT_TRUE(n.type().IsInt32());
}

TEST(NodeTest, ToString) {
  Node n(42, Opcode::kAddI);
  std::string s = n.ToString();

  // Should contain opcode name and ID
  EXPECT_NE(s.find("AddI"), std::string::npos);
  EXPECT_NE(s.find("42"), std::string::npos);
}

// ========== Schema-Aware Accessor Tests ==========

TEST(NodeTest, SchemaClassification) {
  // Test S0: Pure
  Node pure(1, Opcode::kAddI);
  EXPECT_EQ(pure.schema(), NodeSchema::kS0_Pure);

  // Test S1: Control
  Node control(2, Opcode::kIfTrue);
  EXPECT_EQ(control.schema(), NodeSchema::kS1_Control);

  // Test S2: Merge
  Node merge(3, Opcode::kPhi);
  EXPECT_EQ(merge.schema(), NodeSchema::kS2_Merge);

  // Test S3: Load
  Node load(4, Opcode::kLoadI);
  EXPECT_EQ(load.schema(), NodeSchema::kS3_Load);

  // Test S4: Store
  Node store(5, Opcode::kStoreI);
  EXPECT_EQ(store.schema(), NodeSchema::kS4_Store);

  // Test S5: Allocate
  Node alloc(6, Opcode::kAllocate);
  EXPECT_EQ(alloc.schema(), NodeSchema::kS5_Allocate);

  // Test S6: Return
  Node ret(7, Opcode::kReturn);
  EXPECT_EQ(ret.schema(), NodeSchema::kS6_Return);

  // Test S7: Start
  Node start(8, Opcode::kStart);
  EXPECT_EQ(start.schema(), NodeSchema::kS7_Start);
}

TEST(NodeTest, ControlInputAccessor) {
  // Create nodes for Load operation (S3 schema)
  Node ctrl(1, Opcode::kIfTrue);
  Node mem(2, Opcode::kMergeMem);
  Node addr(3, Opcode::kConI);
  Node load(4, Opcode::kLoadI);

  // Set up Load: input[0] = control, input[1] = memory, input[2] = address
  load.set_input(0, &ctrl);
  load.set_input(1, &mem);
  load.set_input(2, &addr);

  // Test control_input accessor
  EXPECT_EQ(load.control_input(), &ctrl);

  // Test on Store (S4)
  Node store(5, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  EXPECT_EQ(store.control_input(), &ctrl);

  // Test on Return (S6)
  Node ret(6, Opcode::kReturn);
  ret.set_input(0, &ctrl);
  EXPECT_EQ(ret.control_input(), &ctrl);

  // Test on pure node (should return nullptr)
  Node pure(7, Opcode::kAddI);
  EXPECT_EQ(pure.control_input(), nullptr);
}

TEST(NodeTest, MemoryInputAccessor) {
  // Create nodes for Load operation
  Node ctrl(1, Opcode::kIfTrue);
  Node mem(2, Opcode::kMergeMem);
  Node addr(3, Opcode::kConI);
  Node load(4, Opcode::kLoadI);

  // Set up Load: input[0] = control, input[1] = memory, input[2] = address
  load.set_input(0, &ctrl);
  load.set_input(1, &mem);
  load.set_input(2, &addr);

  // Test memory_input accessor
  EXPECT_EQ(load.memory_input(), &mem);

  // Test on Store (S4)
  Node value(5, Opcode::kConI);
  Node store(6, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  store.set_input(1, &mem);
  store.set_input(2, &addr);
  store.set_input(3, &value);
  EXPECT_EQ(store.memory_input(), &mem);

  // Test on Allocate (S5)
  Node alloc(7, Opcode::kAllocate);
  alloc.set_input(0, &ctrl);
  alloc.set_input(1, &mem);
  EXPECT_EQ(alloc.memory_input(), &mem);

  // Test on pure node (should return nullptr)
  Node pure(8, Opcode::kAddI);
  EXPECT_EQ(pure.memory_input(), nullptr);
}

TEST(NodeTest, ValueInputsAccessor) {
  // Test pure node (S0) - all inputs are values
  Node a(1, Opcode::kConI);
  Node b(2, Opcode::kConI);
  Node add(3, Opcode::kAddI);
  add.AddInput(&a);
  add.AddInput(&b);

  auto value_inputs = add.value_inputs();
  EXPECT_EQ(value_inputs.size(), 2);
  EXPECT_EQ(value_inputs[0], &a);
  EXPECT_EQ(value_inputs[1], &b);

  // Test Load node (S3) - value inputs skip control and memory
  Node ctrl(4, Opcode::kIfTrue);
  Node mem(5, Opcode::kMergeMem);
  Node addr(6, Opcode::kConI);
  Node load(7, Opcode::kLoadI);
  load.set_input(0, &ctrl);
  load.set_input(1, &mem);
  load.set_input(2, &addr);

  auto load_values = load.value_inputs();
  EXPECT_EQ(load_values.size(), 1);
  EXPECT_EQ(load_values[0], &addr);

  // Test Store node (S4) - value inputs skip control and memory
  Node value(8, Opcode::kConI);
  Node store(9, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  store.set_input(1, &mem);
  store.set_input(2, &addr);
  store.set_input(3, &value);

  auto store_values = store.value_inputs();
  EXPECT_EQ(store_values.size(), 2);
  EXPECT_EQ(store_values[0], &addr);
  EXPECT_EQ(store_values[1], &value);
}

TEST(NodeTest, PhiAccessors) {
  // Create Region with 2 control predecessors
  Node pred1(1, Opcode::kIfTrue);
  Node pred2(2, Opcode::kIfFalse);
  Node region(3, Opcode::kRegion);
  region.AddInput(&pred1);
  region.AddInput(&pred2);

  // Create Phi: input[0] = Region, input[1..n] = values
  Node val1(4, Opcode::kConI);
  Node val2(5, Opcode::kConI);
  Node phi(6, Opcode::kPhi);
  phi.set_input(0, &region);
  phi.set_input(1, &val1);
  phi.set_input(2, &val2);

  // Test region_input accessor
  EXPECT_EQ(phi.region_input(), &region);

  // Test phi_values accessor
  auto phi_vals = phi.phi_values();
  EXPECT_EQ(phi_vals.size(), 2);
  EXPECT_EQ(phi_vals[0], &val1);
  EXPECT_EQ(phi_vals[1], &val2);
}

TEST(NodeTest, RegionAccessors) {
  // Create Region with control predecessors
  Node pred1(1, Opcode::kIfTrue);
  Node pred2(2, Opcode::kIfFalse);
  Node pred3(3, Opcode::kGoto);
  Node region(4, Opcode::kRegion);
  region.AddInput(&pred1);
  region.AddInput(&pred2);
  region.AddInput(&pred3);

  // Test region_preds accessor
  auto preds = region.region_preds();
  EXPECT_EQ(preds.size(), 3);
  EXPECT_EQ(preds[0], &pred1);
  EXPECT_EQ(preds[1], &pred2);
  EXPECT_EQ(preds[2], &pred3);
}

TEST(NodeTest, AddressInputAccessor) {
  // Test Load address input
  Node ctrl(1, Opcode::kIfTrue);
  Node mem(2, Opcode::kMergeMem);
  Node addr(3, Opcode::kAddP);
  Node load(4, Opcode::kLoadI);
  load.set_input(0, &ctrl);
  load.set_input(1, &mem);
  load.set_input(2, &addr);

  EXPECT_EQ(load.address_input(), &addr);

  // Test Store address input
  Node value(5, Opcode::kConI);
  Node store(6, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  store.set_input(1, &mem);
  store.set_input(2, &addr);
  store.set_input(3, &value);

  EXPECT_EQ(store.address_input(), &addr);

  // Test on non-memory node (should return nullptr)
  Node pure(7, Opcode::kAddI);
  EXPECT_EQ(pure.address_input(), nullptr);
}

TEST(NodeTest, StoreValueInputAccessor) {
  // Test Store value input
  Node ctrl(1, Opcode::kIfTrue);
  Node mem(2, Opcode::kMergeMem);
  Node addr(3, Opcode::kAddP);
  Node value(4, Opcode::kConI);
  Node store(5, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  store.set_input(1, &mem);
  store.set_input(2, &addr);
  store.set_input(3, &value);

  EXPECT_EQ(store.store_value_input(), &value);

  // Test on non-store node (should return nullptr)
  Node load(6, Opcode::kLoadI);
  EXPECT_EQ(load.store_value_input(), nullptr);
}

TEST(NodeTest, InputValidation) {
  // Valid Load node
  Node ctrl(1, Opcode::kIfTrue);
  Node mem(2, Opcode::kMergeMem);
  Node addr(3, Opcode::kConI);
  Node load(4, Opcode::kLoadI);
  load.set_input(0, &ctrl);
  load.set_input(1, &mem);
  load.set_input(2, &addr);
  EXPECT_TRUE(load.ValidateInputs());

  // Invalid Load (missing memory input)
  Node bad_load(5, Opcode::kLoadI);
  bad_load.set_input(0, &ctrl);
  EXPECT_FALSE(bad_load.ValidateInputs());

  // Valid Store node
  Node value(6, Opcode::kConI);
  Node store(7, Opcode::kStoreI);
  store.set_input(0, &ctrl);
  store.set_input(1, &mem);
  store.set_input(2, &addr);
  store.set_input(3, &value);
  EXPECT_TRUE(store.ValidateInputs());

  // Invalid Store (missing value input)
  Node bad_store(8, Opcode::kStoreI);
  bad_store.set_input(0, &ctrl);
  bad_store.set_input(1, &mem);
  bad_store.set_input(2, &addr);
  EXPECT_FALSE(bad_store.ValidateInputs());

  // Pure node with any number of inputs should validate
  Node a(9, Opcode::kConI);
  Node b(10, Opcode::kConI);
  Node add(11, Opcode::kAddI);
  add.AddInput(&a);
  add.AddInput(&b);
  EXPECT_TRUE(add.ValidateInputs());
}
