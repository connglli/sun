#include "suntv/ir/node.hpp"

#include <sstream>
#include <stdexcept>

namespace sun {

Node::Node(NodeID id, Opcode opcode)
    : id_(id), opcode_(opcode), type_(TypeKind::kTop) {}

Node* Node::input(size_t i) const {
  if (i >= inputs_.size()) {
    throw std::out_of_range("Input index out of range");
  }
  return inputs_[i];
}

void Node::AddInput(Node* n) { inputs_.push_back(n); }

void Node::set_input(size_t i, Node* n) {
  if (i >= inputs_.size()) {
    inputs_.resize(i + 1, nullptr);
  }
  inputs_[i] = n;
}

bool Node::has_prop(const std::string& key) const {
  return props_.find(key) != props_.end();
}

Property Node::prop(const std::string& key) const {
  auto it = props_.find(key);
  if (it == props_.end()) {
    throw std::runtime_error("Property not found: " + key);
  }
  return it->second;
}

void Node::set_prop(const std::string& key, Property value) {
  props_[key] = value;
}

std::string Node::ToString() const {
  std::ostringstream oss;
  oss << OpcodeToString(opcode_) << " [id=" << id_ << "]";
  return oss.str();
}

}  // namespace sun
