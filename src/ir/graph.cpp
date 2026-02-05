#include "suntv/ir/graph.hpp"

#include <algorithm>

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

Node* Graph::add_node(NodeID id, Opcode op) {
  auto node = std::make_unique<Node>(id, op);
  Node* ptr = node.get();

  owned_nodes_.push_back(std::move(node));
  node_list_.push_back(ptr);
  id_to_node_[id] = ptr;

  // Track special nodes
  if (op == Opcode::Start) {
    start_ = ptr;
  } else if (op == Opcode::Root) {
    root_ = ptr;
  }

  return ptr;
}

std::vector<Node*> Graph::parameter_nodes() const {
  std::vector<Node*> params;
  for (Node* n : node_list_) {
    if (n->opcode() == Opcode::Parm) {
      params.push_back(n);
    }
  }
  return params;
}

std::vector<Node*> Graph::control_nodes() const {
  std::vector<Node*> controls;
  for (Node* n : node_list_) {
    if (is_control(n->opcode())) {
      controls.push_back(n);
    }
  }
  return controls;
}

}  // namespace sun
