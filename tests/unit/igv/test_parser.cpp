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

  auto graph = parser.parse(path);
  ASSERT_NE(graph, nullptr) << "Parser should return a valid graph";

  // Verify node count: Root(0), Start(1), ConI(10), Return(20) = 4 nodes
  EXPECT_EQ(graph->nodes().size(), 4);

  // Verify Root node exists
  Node* root = graph->node(0);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->opcode(), Opcode::Root);

  // Verify Start node exists
  Node* start = graph->node(1);
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->opcode(), Opcode::Start);

  // Verify ConI node exists with value 42
  Node* conI = graph->node(10);
  ASSERT_NE(conI, nullptr);
  EXPECT_EQ(conI->opcode(), Opcode::ConI);
  EXPECT_TRUE(conI->has_prop("value"));
  // Value is stored as int32_t since it's numeric
  EXPECT_EQ(std::get<int32_t>(conI->get_prop("value")), 42);

  // Verify Return node exists
  Node* ret = graph->node(20);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->opcode(), Opcode::Return);
}

TEST(IGVParserTest, ParseSimpleReturnEdges) {
  IGVParser parser;
  std::string path = getFixturePath("simple_return.xml");

  auto graph = parser.parse(path);
  ASSERT_NE(graph, nullptr);

  // Return node should have 2 inputs: control from Start, value from ConI
  Node* ret = graph->node(20);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->num_inputs(), 2);

  // Input 0: control from Start (node 1)
  EXPECT_EQ(ret->input(0)->id(), 1);
  EXPECT_EQ(ret->input(0)->opcode(), Opcode::Start);

  // Input 1: value from ConI (node 10)
  EXPECT_EQ(ret->input(1)->id(), 10);
  EXPECT_EQ(ret->input(1)->opcode(), Opcode::ConI);
}

TEST(IGVParserTest, ParseSimpleAdd) {
  IGVParser parser;
  std::string path = getFixturePath("simple_add.xml");

  auto graph = parser.parse(path);
  ASSERT_NE(graph, nullptr);

  // Verify node count: Root(0), Start(1), Parm(10), Parm(11), AddI(20),
  // Return(30) = 6 nodes
  EXPECT_EQ(graph->nodes().size(), 6);

  // Verify Start node
  Node* start = graph->node(1);
  ASSERT_NE(start, nullptr);
  EXPECT_EQ(start->opcode(), Opcode::Start);

  // Verify first Parm node
  Node* parm0 = graph->node(10);
  ASSERT_NE(parm0, nullptr);
  EXPECT_EQ(parm0->opcode(), Opcode::Parm);
  EXPECT_TRUE(parm0->has_prop("index"));
  EXPECT_EQ(std::get<int32_t>(parm0->get_prop("index")), 0);
  EXPECT_TRUE(parm0->has_prop("type"));
  EXPECT_EQ(std::get<std::string>(parm0->get_prop("type")), "int");

  // Verify second Parm node
  Node* parm1 = graph->node(11);
  ASSERT_NE(parm1, nullptr);
  EXPECT_EQ(parm1->opcode(), Opcode::Parm);
  EXPECT_TRUE(parm1->has_prop("index"));
  EXPECT_EQ(std::get<int32_t>(parm1->get_prop("index")), 1);

  // Verify AddI node
  Node* addI = graph->node(20);
  ASSERT_NE(addI, nullptr);
  EXPECT_EQ(addI->opcode(), Opcode::AddI);

  // Verify Return node
  Node* ret = graph->node(30);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->opcode(), Opcode::Return);
}

TEST(IGVParserTest, ParseSimpleAddEdges) {
  IGVParser parser;
  std::string path = getFixturePath("simple_add.xml");

  auto graph = parser.parse(path);
  ASSERT_NE(graph, nullptr);

  // AddI node should have 2 inputs: both Parm nodes
  Node* addI = graph->node(20);
  ASSERT_NE(addI, nullptr);
  EXPECT_EQ(addI->num_inputs(), 2);

  // Input 0: Parm node 10
  EXPECT_EQ(addI->input(0)->id(), 10);
  EXPECT_EQ(addI->input(0)->opcode(), Opcode::Parm);

  // Input 1: Parm node 11
  EXPECT_EQ(addI->input(1)->id(), 11);
  EXPECT_EQ(addI->input(1)->opcode(), Opcode::Parm);

  // Return node should have 2 inputs: control from Start, value from AddI
  Node* ret = graph->node(30);
  ASSERT_NE(ret, nullptr);
  EXPECT_EQ(ret->num_inputs(), 2);

  // Input 0: control from Start
  EXPECT_EQ(ret->input(0)->id(), 1);
  EXPECT_EQ(ret->input(0)->opcode(), Opcode::Start);

  // Input 1: value from AddI
  EXPECT_EQ(ret->input(1)->id(), 20);
  EXPECT_EQ(ret->input(1)->opcode(), Opcode::AddI);
}

TEST(IGVParserTest, ParseNonexistentFile) {
  IGVParser parser;
  std::string path = "/nonexistent/file.xml";

  // Should return nullptr on error
  auto graph = parser.parse(path);
  EXPECT_EQ(graph, nullptr);
}
