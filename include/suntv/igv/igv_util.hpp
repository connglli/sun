#pragma once

#include <string>
#include <vector>

namespace sun {

/**
 * Graph information from an IGV XML file.
 */
struct GraphInfo {
  std::string name;
  size_t num_nodes;
  size_t num_edges;
  size_t index;  // Index within the file (0-based)
};

/**
 * IGV Utility class for listing and extracting graphs from IGV XML files.
 */
class IGVUtil {
 public:
  /**
   * List all graphs in an IGV XML file.
   * Returns a vector of GraphInfo containing name, node count, and edge count.
   */
  static std::vector<GraphInfo> ListGraphs(const std::string& path);

  /**
   * Extract a graph by index from an IGV XML file and save as a separate IGV
   * XML file.
   * @param input_path Path to input IGV XML file
   * @param graph_index Index of graph to extract (0-based)
   * @param output_path Path to output IGV XML file
   * @return true on success, false on failure
   */
  static bool ExtractGraph(const std::string& input_path, size_t graph_index,
                           const std::string& output_path);

  /**
   * Extract a graph by name from an IGV XML file and save as a separate IGV
   * XML file.
   * @param input_path Path to input IGV XML file
   * @param graph_name Name of graph to extract
   * @param output_path Path to output IGV XML file
   * @return true on success, false on failure
   */
  static bool ExtractGraph(const std::string& input_path,
                           const std::string& graph_name,
                           const std::string& output_path);
};

}  // namespace sun
