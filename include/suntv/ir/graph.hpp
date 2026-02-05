#pragma once

#include <map>
#include <memory>
#include <vector>

#include "suntv/ir/node.hpp"

namespace sun {

/**
 * Graph container for Sea-of-Nodes IR.
 */
class Graph {
 public:
  Graph();
  ~Graph();

  // Node access
  Node* node(NodeID id) const;
  Node* start() const { return start_; }
  Node* root() const { return root_; }
  const std::vector<Node*>& nodes() const { return node_list_; }

  // Node creation
  Node* add_node(NodeID id, Opcode op);

  // Graph queries
  std::vector<Node*> parameter_nodes() const;
  std::vector<Node*> control_nodes() const;

  // Debugging
  void dump() const;  // Print graph structure to stdout

 private:
  std::vector<std::unique_ptr<Node>> owned_nodes_;
  std::vector<Node*> node_list_;  // All nodes (for iteration)
  std::map<NodeID, Node*> id_to_node_;
  Node* start_;
  Node* root_;
};

}  // namespace sun
