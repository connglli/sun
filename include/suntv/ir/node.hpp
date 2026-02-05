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

  // Schema-aware accessors
  NodeSchema schema() const;

  // Control input (valid for S1, S3, S4, S5, S6)
  Node* control_input() const;

  // Memory input (valid for S3, S4, S5, S6)
  Node* memory_input() const;

  // Value inputs (skips control/memory based on schema)
  std::vector<Node*> value_inputs() const;
  size_t num_value_inputs() const;

  // Schema-specific accessors
  Node* region_input() const;             // For Phi (S2) - returns input[0]
  std::vector<Node*> phi_values() const;  // For Phi (S2) - returns input[1..n]
  std::vector<Node*> region_preds()
      const;  // For Region (S2) - returns all inputs

  Node* address_input()
      const;  // For Load/Store (S3, S4) - returns address input
  Node* store_value_input() const;  // For Store (S4) - returns value to store

  // Validation
  bool ValidateInputs() const;  // Check if inputs match schema requirements

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
