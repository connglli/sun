#include <gtest/gtest.h>

#include "suntv/ir/opcode.hpp"

using namespace sun;

TEST(OpcodeTest, OpcodeToString) {
  EXPECT_EQ(OpcodeToString(Opcode::Start), "Start");
  EXPECT_EQ(OpcodeToString(Opcode::AddI), "AddI");
  EXPECT_EQ(OpcodeToString(Opcode::ConI), "ConI");
  EXPECT_EQ(OpcodeToString(Opcode::Return), "Return");
  EXPECT_EQ(OpcodeToString(Opcode::Unknown), "Unknown");
}

TEST(OpcodeTest, StringToOpcode) {
  EXPECT_EQ(StringToOpcode("Start"), Opcode::Start);
  EXPECT_EQ(StringToOpcode("AddI"), Opcode::AddI);
  EXPECT_EQ(StringToOpcode("ConI"), Opcode::ConI);
  EXPECT_EQ(StringToOpcode("Return"), Opcode::Return);
  EXPECT_EQ(StringToOpcode("InvalidOpcode"), Opcode::Unknown);
  EXPECT_EQ(StringToOpcode(""), Opcode::Unknown);
}

TEST(OpcodeTest, IsControl) {
  EXPECT_TRUE(IsControl(Opcode::Start));
  EXPECT_TRUE(IsControl(Opcode::If));
  EXPECT_TRUE(IsControl(Opcode::IfTrue));
  EXPECT_TRUE(IsControl(Opcode::Region));
  EXPECT_TRUE(IsControl(Opcode::Return));

  EXPECT_FALSE(IsControl(Opcode::AddI));
  EXPECT_FALSE(IsControl(Opcode::Phi));
  EXPECT_FALSE(IsControl(Opcode::LoadI));
}

TEST(OpcodeTest, IsPure) {
  EXPECT_TRUE(IsPure(Opcode::AddI));
  EXPECT_TRUE(IsPure(Opcode::SubL));
  EXPECT_TRUE(IsPure(Opcode::ConI));
  EXPECT_TRUE(IsPure(Opcode::CmpI));

  EXPECT_FALSE(IsPure(Opcode::LoadI));
  EXPECT_FALSE(IsPure(Opcode::StoreI));
  EXPECT_FALSE(IsPure(Opcode::Allocate));
  EXPECT_FALSE(IsPure(Opcode::Return));
}

TEST(OpcodeTest, IsMemory) {
  EXPECT_TRUE(IsMemory(Opcode::LoadI));
  EXPECT_TRUE(IsMemory(Opcode::StoreL));
  EXPECT_TRUE(IsMemory(Opcode::Allocate));
  EXPECT_TRUE(IsMemory(Opcode::MergeMem));

  EXPECT_FALSE(IsMemory(Opcode::AddI));
  EXPECT_FALSE(IsMemory(Opcode::Return));
}
