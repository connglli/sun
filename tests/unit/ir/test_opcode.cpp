#include <gtest/gtest.h>

#include "suntv/ir/opcode.hpp"

using namespace sun;

TEST(OpcodeTest, OpcodeToString) {
  EXPECT_EQ(opcode_to_string(Opcode::Start), "Start");
  EXPECT_EQ(opcode_to_string(Opcode::AddI), "AddI");
  EXPECT_EQ(opcode_to_string(Opcode::ConI), "ConI");
  EXPECT_EQ(opcode_to_string(Opcode::Return), "Return");
  EXPECT_EQ(opcode_to_string(Opcode::Unknown), "Unknown");
}

TEST(OpcodeTest, StringToOpcode) {
  EXPECT_EQ(string_to_opcode("Start"), Opcode::Start);
  EXPECT_EQ(string_to_opcode("AddI"), Opcode::AddI);
  EXPECT_EQ(string_to_opcode("ConI"), Opcode::ConI);
  EXPECT_EQ(string_to_opcode("Return"), Opcode::Return);
  EXPECT_EQ(string_to_opcode("InvalidOpcode"), Opcode::Unknown);
  EXPECT_EQ(string_to_opcode(""), Opcode::Unknown);
}

TEST(OpcodeTest, IsControl) {
  EXPECT_TRUE(is_control(Opcode::Start));
  EXPECT_TRUE(is_control(Opcode::If));
  EXPECT_TRUE(is_control(Opcode::IfTrue));
  EXPECT_TRUE(is_control(Opcode::Region));
  EXPECT_TRUE(is_control(Opcode::Return));

  EXPECT_FALSE(is_control(Opcode::AddI));
  EXPECT_FALSE(is_control(Opcode::Phi));
  EXPECT_FALSE(is_control(Opcode::LoadI));
}

TEST(OpcodeTest, IsPure) {
  EXPECT_TRUE(is_pure(Opcode::AddI));
  EXPECT_TRUE(is_pure(Opcode::SubL));
  EXPECT_TRUE(is_pure(Opcode::ConI));
  EXPECT_TRUE(is_pure(Opcode::CmpI));

  EXPECT_FALSE(is_pure(Opcode::LoadI));
  EXPECT_FALSE(is_pure(Opcode::StoreI));
  EXPECT_FALSE(is_pure(Opcode::Allocate));
  EXPECT_FALSE(is_pure(Opcode::Return));
}

TEST(OpcodeTest, IsMemory) {
  EXPECT_TRUE(is_memory(Opcode::LoadI));
  EXPECT_TRUE(is_memory(Opcode::StoreL));
  EXPECT_TRUE(is_memory(Opcode::Allocate));
  EXPECT_TRUE(is_memory(Opcode::MergeMem));

  EXPECT_FALSE(is_memory(Opcode::AddI));
  EXPECT_FALSE(is_memory(Opcode::Return));
}
