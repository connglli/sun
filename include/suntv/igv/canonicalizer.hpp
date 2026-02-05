#pragma once

#include <string>

namespace sun {
class Graph;

/**
 * Canonicalizer: post-processes a parsed IGV graph.
 *
 * Responsibilities:
 * - Validate well-formedness (single Start/Root, acyclicity, etc.)
 * - Set special node pointers (start_, root_) in Graph
 * - Future: Type inference, comparison normalization
 *
 * Returns nullptr if the graph is malformed.
 */
class Canonicalizer {
 public:
  Canonicalizer() = default;

  /**
   * Canonicalize and validate a parsed graph.
   * Returns the same graph pointer on success, nullptr on failure.
   * Error messages are logged.
   */
  Graph* Canonicalize(Graph* raw);

 private:
  bool ValidateWellFormed(Graph* g, std::string& error);
  bool CheckSingleStartRoot(Graph* g, std::string& error);
  // Future: CheckAcyclicity, InferTypes, NormalizeComparisons
};

}  // namespace sun
