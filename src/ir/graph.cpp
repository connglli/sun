#include "suntv/ir/graph.hpp"

#include <algorithm>
#include <iostream>

#include "suntv/ir/opcode.hpp"

namespace sun {

Graph::Graph() : start_(nullptr), root_(nullptr) {}

Graph::~Graph() = default;

Node* Graph::node(NodeID id) const {
  auto it = id_to_node_.find(id);
  if (it == id_to_node_.end()) {
    return nullptr;
  }
  return it->second;
}

Node* Graph::AddNode(NodeID id, Opcode op) {
  auto node = std::make_unique<Node>(id, op);
  Node* ptr = node.get();

  owned_nodes_.push_back(std::move(node));
  node_list_.push_back(ptr);
  id_to_node_[id] = ptr;

  // Track special nodes
  if (op == Opcode::kStart) {
    start_ = ptr;
  } else if (op == Opcode::kRoot) {
    root_ = ptr;
  }

  return ptr;
}

std::vector<Node*> Graph::GetParameterNodes() const {
  std::vector<Node*> params;
  for (Node* n : node_list_) {
    if (n->opcode() == Opcode::kParm) {
      params.push_back(n);
    }
  }
  return params;
}

std::vector<Node*> Graph::GetControlNodes() const {
  std::vector<Node*> controls;
  for (Node* n : node_list_) {
    if (IsControl(n->opcode())) {
      controls.push_back(n);
    }
  }
  return controls;
}

void Graph::Dump() const {
  std::cout << "=== Graph Dump ===" << std::endl;
  std::cout << "Total nodes: " << node_list_.size() << std::endl;
  std::cout << "Start: " << (start_ ? std::to_string(start_->id()) : "none")
            << std::endl;
  std::cout << "Root: " << (root_ ? std::to_string(root_->id()) : "none")
            << std::endl;
  std::cout << std::endl;

  // Sort nodes by ID for consistent output
  std::vector<Node*> sorted_nodes = node_list_;
  std::sort(sorted_nodes.begin(), sorted_nodes.end(),
            [](Node* a, Node* b) { return a->id() < b->id(); });

  for (Node* n : sorted_nodes) {
    std::cout << n->ToString() << std::endl;
  }

  std::cout << "==================" << std::endl;
}

}  // namespace sun
