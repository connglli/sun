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
  Node* GetNode(NodeID id) const;
  Node* GetStart() const { return start_; }
  Node* GetRoot() const { return root_; }
  const std::vector<Node*>& GetNodes() const { return node_list_; }

  // Node creation
  Node* AddNode(NodeID id, Opcode op);

  // Graph queries
  std::vector<Node*> GetParameterNodes() const;
  std::vector<Node*> GetControlNodes() const;

  // Debugging
  void Dump() const;  // Print graph structure to stdout

 private:
  std::vector<std::unique_ptr<Node>> owned_nodes_;
  std::vector<Node*> node_list_;  // All nodes (for iteration)
  std::map<NodeID, Node*> id_to_node_;
  Node* start_;
  Node* root_;
};

}  // namespace sun
