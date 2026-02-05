#pragma once
#include <memory>
#include <string>

namespace sun {
class Graph;

/**
 * IGV XML Parser.
 * Parses Ideal Graph Visualizer XML format into our internal Graph IR.
 */
class IGVParser {
 public:
  IGVParser();
  ~IGVParser();

  /**
   * Parse an IGV XML file and return a Graph.
   * Returns nullptr on parse error.
   */
  std::unique_ptr<Graph> Parse(const std::string& path);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace sun
