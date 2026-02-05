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

  NodeID Id() const { return id_; }
  Opcode GetOpcode() const { return opcode_; }

  // Inputs (edges)
  size_t NumInputs() const { return inputs_.size(); }
  Node* GetInput(size_t i) const;
  void AddInput(Node* n);
  void SetInput(size_t i, Node* n);

  // Properties
  bool HasProp(const std::string& key) const;
  Property GetProp(const std::string& key) const;
  void SetProp(const std::string& key, Property value);

  // Type
  TypeStamp GetType() const { return type_; }
  void SetType(TypeStamp type) { type_ = type; }

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
