#include "suntv/igv/canonicalizer.hpp"

#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

Graph* Canonicalizer::Canonicalize(Graph* raw) {
  if (!raw) {
    return nullptr;
  }

  std::string error;
  if (!ValidateWellFormed(raw, error)) {
    Logger::Error("Graph validation failed: " + error);
    return nullptr;
  }

  Logger::Info("Graph canonicalization successful");
  return raw;
}

bool Canonicalizer::ValidateWellFormed(Graph* g, std::string& error) {
  // Check for single Start and Root nodes
  if (!CheckSingleStartRoot(g, error)) {
    return false;
  }

  // Future validations:
  // - Check acyclicity (except for Region/Phi)
  // - Validate Phi nodes correspond to Region nodes
  // - Check that all nodes are reachable from Start
  // - Verify edge consistency (input count matches opcode requirements)

  return true;
}

bool Canonicalizer::CheckSingleStartRoot(Graph* g, std::string& error) {
  Node* start_node = nullptr;
  Node* root_node = nullptr;

  for (Node* n : g->nodes()) {
    if (n->opcode() == Opcode::kStart) {
      if (start_node) {
        error = "Multiple Start nodes found (IDs: " +
                std::to_string(start_node->id()) + ", " +
                std::to_string(n->id()) + ")";
        return false;
      }
      start_node = n;
    }

    if (n->opcode() == Opcode::kRoot) {
      if (root_node) {
        error = "Multiple Root nodes found (IDs: " +
                std::to_string(root_node->id()) + ", " +
                std::to_string(n->id()) + ")";
        return false;
      }
      root_node = n;
    }
  }

  if (!start_node) {
    error = "No Start node found";
    return false;
  }

  if (!root_node) {
    error = "No Root node found";
    return false;
  }

  // Note: We don't set g's start_ and root_ pointers here because Graph's
  // constructor or AddNode should handle that automatically when Start/Root
  // nodes are added. If needed, we could add setter methods to Graph and
  // call them here.

  Logger::Debug("Found Start node (ID " + std::to_string(start_node->id()) +
                ") and Root node (ID " + std::to_string(root_node->id()) + ")");

  return true;
}

}  // namespace sun
