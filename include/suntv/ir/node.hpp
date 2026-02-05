#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "suntv/ir/opcode.hpp"
#include "suntv/ir/types.hpp"

namespace sun {

using NodeID = int32_t;

/**
 * Property value (constants, field IDs, etc.)
 */
using Property = std::variant<int32_t, int64_t, std::string, bool>;

/**
 * Node in the Sea-of-Nodes IR.
 */
class Node {
 public:
  Node(NodeID id, Opcode opcode);

  NodeID id() const { return id_; }
  Opcode opcode() const { return opcode_; }

  // Inputs (edges)
  size_t num_inputs() const { return inputs_.size(); }
  Node* input(size_t i) const;
  void AddInput(Node* n);
  void set_input(size_t i, Node* n);

  // Properties
  bool has_prop(const std::string& key) const;
  Property prop(const std::string& key) const;
  void set_prop(const std::string& key, Property value);

  // Type
  TypeStamp type() const { return type_; }
  void set_type(TypeStamp t) { type_ = t; }

  // Debugging
  std::string ToString() const;

 private:
  NodeID id_;
  Opcode opcode_;
  std::vector<Node*> inputs_;
  std::map<std::string, Property> props_;
  TypeStamp type_;
};

}  // namespace sun
