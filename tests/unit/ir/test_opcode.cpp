#include <gtest/gtest.h>

#include "suntv/ir/opcode.hpp"

using namespace sun;

TEST(OpcodeTest, OpcodeToString) {
  EXPECT_EQ(OpcodeToString(Opcode::kStart), "Start");
  EXPECT_EQ(OpcodeToString(Opcode::kAddI), "AddI");
  EXPECT_EQ(OpcodeToString(Opcode::kConI), "ConI");
  EXPECT_EQ(OpcodeToString(Opcode::kReturn), "Return");
  EXPECT_EQ(OpcodeToString(Opcode::kUnknown), "Unknown");
}

TEST(OpcodeTest, StringToOpcode) {
  EXPECT_EQ(StringToOpcode("Start"), Opcode::kStart);
  EXPECT_EQ(StringToOpcode("AddI"), Opcode::kAddI);
  EXPECT_EQ(StringToOpcode("ConI"), Opcode::kConI);
  EXPECT_EQ(StringToOpcode("Return"), Opcode::kReturn);
  EXPECT_EQ(StringToOpcode("InvalidOpcode"), Opcode::kUnknown);
  EXPECT_EQ(StringToOpcode(""), Opcode::kUnknown);
}

TEST(OpcodeTest, IsControl) {
  EXPECT_TRUE(IsControl(Opcode::kStart));
  EXPECT_TRUE(IsControl(Opcode::kIf));
  EXPECT_TRUE(IsControl(Opcode::kIfTrue));
  EXPECT_TRUE(IsControl(Opcode::kRegion));
  EXPECT_TRUE(IsControl(Opcode::kReturn));

  EXPECT_FALSE(IsControl(Opcode::kAddI));
  EXPECT_FALSE(IsControl(Opcode::kPhi));
  EXPECT_FALSE(IsControl(Opcode::kLoadI));
}

TEST(OpcodeTest, IsPure) {
  EXPECT_TRUE(IsPure(Opcode::kAddI));
  EXPECT_TRUE(IsPure(Opcode::kSubL));
  EXPECT_TRUE(IsPure(Opcode::kConI));
  EXPECT_TRUE(IsPure(Opcode::kCmpI));

  EXPECT_FALSE(IsPure(Opcode::kLoadI));
  EXPECT_FALSE(IsPure(Opcode::kStoreI));
  EXPECT_FALSE(IsPure(Opcode::kAllocate));
  EXPECT_FALSE(IsPure(Opcode::kReturn));
}

TEST(OpcodeTest, IsMemory) {
  EXPECT_TRUE(IsMemory(Opcode::kLoadI));
  EXPECT_TRUE(IsMemory(Opcode::kStoreL));
  EXPECT_TRUE(IsMemory(Opcode::kAllocate));
  EXPECT_TRUE(IsMemory(Opcode::kMergeMem));

  EXPECT_FALSE(IsMemory(Opcode::kAddI));
  EXPECT_FALSE(IsMemory(Opcode::kReturn));
}
