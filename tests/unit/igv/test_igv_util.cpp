#include <gtest/gtest.h>

#include <filesystem>

#include "suntv/igv/igv_util.hpp"

using namespace sun;
namespace fs = std::filesystem;

// Helper to get test fixture path
static std::string getFixturePath(const std::string& filename) {
#ifndef SUN_TEST_FIXTURE_DIR
#define SUN_TEST_FIXTURE_DIR "tests/fixtures"
#endif
  return std::string(SUN_TEST_FIXTURE_DIR) + "/" + filename;
}

TEST(IGVUtilTest, ListGraphsFromSingleGraphFile) {
  std::string path = getFixturePath("simple_add.xml");

  auto graphs = IGVUtil::ListGraphs(path);

  // simple_add.xml has only one graph named "After Parsing"
  ASSERT_EQ(graphs.size(), 1);
  EXPECT_EQ(graphs[0].name, "After Parsing");
  EXPECT_GT(graphs[0].num_nodes, 0);
  EXPECT_GT(graphs[0].num_edges, 0);
}

TEST(IGVUtilTest, ListGraphsFromMultiGraphFile) {
  std::string path = getFixturePath("igv/Fibonacci.xml");

  auto graphs = IGVUtil::ListGraphs(path);

  // Fibonacci.xml has many graphs at different optimization stages
  EXPECT_GT(graphs.size(), 5);

  // Check that first graph is "After Parsing"
  bool found_after_parsing = false;
  for (const auto& g : graphs) {
    if (g.name == "After Parsing") {
      found_after_parsing = true;
      EXPECT_GT(g.num_nodes, 0);
      EXPECT_GT(g.num_edges, 0);
    }
  }
  EXPECT_TRUE(found_after_parsing);
}

TEST(IGVUtilTest, ListGraphsNonexistentFile) {
  std::string path = "/nonexistent/file.xml";

  // Should throw or return empty vector on error
  auto graphs = IGVUtil::ListGraphs(path);
  EXPECT_EQ(graphs.size(), 0);
}

TEST(IGVUtilTest, ExtractGraphFromSingleGraphFile) {
  std::string path = getFixturePath("simple_add.xml");
  std::string output_path = "/tmp/test_extracted_simple_add.xml";

  // Extract the first graph (index 0)
  bool result = IGVUtil::ExtractGraph(path, 0, output_path);
  ASSERT_TRUE(result);

  // Verify the extracted file exists and is valid XML
  EXPECT_TRUE(fs::exists(output_path));

  // Clean up
  fs::remove(output_path);
}

TEST(IGVUtilTest, ExtractGraphByName) {
  std::string path = getFixturePath("igv/Fibonacci.xml");
  std::string output_path = "/tmp/test_extracted_fibonacci.xml";

  // Extract graph by name
  bool result = IGVUtil::ExtractGraph(path, "After Parsing", output_path);
  ASSERT_TRUE(result);

  // Verify the extracted file exists
  EXPECT_TRUE(fs::exists(output_path));

  // Verify we can parse the extracted graph
  auto graphs = IGVUtil::ListGraphs(output_path);
  ASSERT_EQ(graphs.size(), 1);
  EXPECT_EQ(graphs[0].name, "After Parsing");

  // Clean up
  fs::remove(output_path);
}

TEST(IGVUtilTest, ExtractGraphInvalidIndex) {
  std::string path = getFixturePath("simple_add.xml");
  std::string output_path = "/tmp/test_extracted_invalid.xml";

  // Try to extract graph at index 10 (doesn't exist)
  bool result = IGVUtil::ExtractGraph(path, 10, output_path);
  EXPECT_FALSE(result);
}

TEST(IGVUtilTest, ExtractGraphNonexistentName) {
  std::string path = getFixturePath("igv/Fibonacci.xml");
  std::string output_path = "/tmp/test_extracted_invalid.xml";

  // Try to extract graph with non-existent name
  bool result = IGVUtil::ExtractGraph(path, "NonExistentGraph", output_path);
  EXPECT_FALSE(result);
}
