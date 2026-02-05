#include <gtest/gtest.h>

#include <filesystem>

#include "suntv/igv/parser.hpp"
#include "suntv/ir/graph.hpp"

using namespace sun;
namespace fs = std::filesystem;

// Helper to get test fixture path
static std::string getFixturePath(const std::string& filename) {
  // Use absolute path from project root
  // This works whether run from build/ or build/tests/
  return "/zdata/projects/sontv/sun/tests/fixtures/" + filename;
}

TEST(IGVParserTest, ParseSimpleReturn) {
  IGVParser parser;
  std::string path = getFixturePath("simple_return.xml");

  auto graph = parser.Parse(path);
  ASSERT_NE(graph, nullptr) << "Parser should return a valid graph";

  // Verify node count: Root(0), Start(1), ConI(10), Return(20) = 4 nodes
  EXPECT_EQ(graph->GetNodes().size(), 4);

  // Verify Root node exists
  Node* root = graph->GetNode(0);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->GetOpcode(), Opcode::Root);

  // Verify Start node exists
  Node* start = graph->GetNode(1);
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->GetOpcode(), Opcode::Start);

  // Verify ConI node exists with value 42
  Node* conI = graph->GetNode(10);
  ASSERT_NE(conI, nullptr);
  EXPECT_EQ(conI->GetOpcode(), Opcode::ConI);
  EXPECT_TRUE(conI->HasProp("value"));
  // Value is stored as int32_t since it's numeric
  EXPECT_EQ(std::get<int32_t>(conI->GetProp("value")), 42);

  // Verify Return node exists
  Node* ret = graph->GetNode(20);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->GetOpcode(), Opcode::Return);
}

TEST(IGVParserTest, ParseSimpleReturnEdges) {
  IGVParser parser;
  std::string path = getFixturePath("simple_return.xml");

  auto graph = parser.Parse(path);
  ASSERT_NE(graph, nullptr);

  // Return node should have 2 inputs: control from Start, value from ConI
  Node* ret = graph->GetNode(20);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->NumInputs(), 2);

  // Input 0: control from Start (node 1)
  EXPECT_EQ(ret->GetInput(0)->Id(), 1);
  EXPECT_EQ(ret->GetInput(0)->GetOpcode(), Opcode::Start);

  // Input 1: value from ConI (node 10)
  EXPECT_EQ(ret->GetInput(1)->Id(), 10);
  EXPECT_EQ(ret->GetInput(1)->GetOpcode(), Opcode::ConI);
}

TEST(IGVParserTest, ParseSimpleAdd) {
  IGVParser parser;
  std::string path = getFixturePath("simple_add.xml");

  auto graph = parser.Parse(path);
  ASSERT_NE(graph, nullptr);

  // Verify node count: Root(0), Start(1), Parm(10), Parm(11), AddI(20),
  // Return(30) = 6 nodes
  EXPECT_EQ(graph->GetNodes().size(), 6);

  // Verify Start node
  Node* start = graph->GetNode(1);
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->GetOpcode(), Opcode::Start);

  // Verify first Parm node
  Node* parm0 = graph->GetNode(10);
  ASSERT_NE(parm0, nullptr);
  EXPECT_EQ(parm0->GetOpcode(), Opcode::Parm);
  EXPECT_TRUE(parm0->HasProp("index"));
  EXPECT_EQ(std::get<int32_t>(parm0->GetProp("index")), 0);
  EXPECT_TRUE(parm0->HasProp("type"));
  EXPECT_EQ(std::get<std::string>(parm0->GetProp("type")), "int");

  // Verify second Parm node
  Node* parm1 = graph->GetNode(11);
  ASSERT_NE(parm1, nullptr);
  EXPECT_EQ(parm1->GetOpcode(), Opcode::Parm);
  EXPECT_TRUE(parm1->HasProp("index"));
  EXPECT_EQ(std::get<int32_t>(parm1->GetProp("index")), 1);

  // Verify AddI node
  Node* addI = graph->GetNode(20);
  ASSERT_NE(addI, nullptr);
  EXPECT_EQ(addI->GetOpcode(), Opcode::AddI);

  // Verify Return node
  Node* ret = graph->GetNode(30);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->GetOpcode(), Opcode::Return);
}

TEST(IGVParserTest, ParseSimpleAddEdges) {
  IGVParser parser;
  std::string path = getFixturePath("simple_add.xml");

  auto graph = parser.Parse(path);
  ASSERT_NE(graph, nullptr);

  // AddI node should have 2 inputs: both Parm nodes
  Node* addI = graph->GetNode(20);
  ASSERT_NE(addI, nullptr);
  EXPECT_EQ(addI->NumInputs(), 2);

  // Input 0: Parm node 10
  EXPECT_EQ(addI->GetInput(0)->Id(), 10);
  EXPECT_EQ(addI->GetInput(0)->GetOpcode(), Opcode::Parm);

  // Input 1: Parm node 11
  EXPECT_EQ(addI->GetInput(1)->Id(), 11);
  EXPECT_EQ(addI->GetInput(1)->GetOpcode(), Opcode::Parm);

  // Return node should have 2 inputs: control from Start, value from AddI
  Node* ret = graph->GetNode(30);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->NumInputs(), 2);

  // Input 0: control from Start
  EXPECT_EQ(ret->GetInput(0)->Id(), 1);
  EXPECT_EQ(ret->GetInput(0)->GetOpcode(), Opcode::Start);

  // Input 1: value from AddI
  EXPECT_EQ(ret->GetInput(1)->Id(), 20);
  EXPECT_EQ(ret->GetInput(1)->GetOpcode(), Opcode::AddI);
}

TEST(IGVParserTest, ParseNonexistentFile) {
  IGVParser parser;
  std::string path = "/nonexistent/file.xml";

  // Should return nullptr on error
  auto graph = parser.Parse(path);
  EXPECT_EQ(graph, nullptr);
}
