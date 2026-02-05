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

// ========== Schema-Aware Accessors ==========

NodeSchema Node::schema() const { return GetSchema(opcode_); }

Node* Node::control_input() const {
  auto s = schema();
  switch (s) {
    case NodeSchema::kS1_Control:
    case NodeSchema::kS3_Load:
    case NodeSchema::kS4_Store:
    case NodeSchema::kS5_Allocate:
    case NodeSchema::kS6_Return:
      // Control input is at index 0 for these schemas
      if (num_inputs() > 0) {
        return input(0);
      }
      return nullptr;
    default:
      return nullptr;
  }
}

Node* Node::memory_input() const {
  auto s = schema();
  switch (s) {
    case NodeSchema::kS3_Load:
    case NodeSchema::kS4_Store:
    case NodeSchema::kS5_Allocate:
    case NodeSchema::kS6_Return:
      // Memory input is at index 1 for these schemas
      if (num_inputs() > 1) {
        return input(1);
      }
      return nullptr;
    default:
      return nullptr;
  }
}

std::vector<Node*> Node::value_inputs() const {
  std::vector<Node*> result;
  auto s = schema();

  size_t start_idx = 0;
  switch (s) {
    case NodeSchema::kS0_Pure:
      // All inputs are values
      start_idx = 0;
      break;

    case NodeSchema::kS1_Control:
      // Control nodes may have a condition input after control
      // input[0] = control, input[1+] = condition/values
      start_idx = 1;
      break;

    case NodeSchema::kS2_Merge:
      // For Phi: input[0] = Region, input[1..n] = values
      // For Region/MergeMem: all inputs are control/memory (not "values" in the
      // traditional sense)
      if (opcode_ == Opcode::kPhi) {
        start_idx = 1;
      } else {
        // Region/MergeMem don't have "value" inputs in the S0 sense
        return result;
      }
      break;

    case NodeSchema::kS3_Load:
      // input[0] = control, input[1] = memory, input[2+] = address/values
      start_idx = 2;
      break;

    case NodeSchema::kS4_Store:
      // input[0] = control, input[1] = memory, input[2+] = address and value
      start_idx = 2;
      break;

    case NodeSchema::kS5_Allocate:
      // input[0] = control, input[1] = memory, input[2+] = size/properties
      start_idx = 2;
      break;

    case NodeSchema::kS6_Return:
      // input[0] = control, input[1+] = various (memory, values)
      start_idx = 1;
      break;

    case NodeSchema::kS9_Parameter:
      // Parm: input[0] = Start node
      start_idx = 1;
      break;

    default:
      // Unknown schema or no value inputs
      return result;
  }

  for (size_t i = start_idx; i < num_inputs(); ++i) {
    if (inputs_[i] != nullptr) {
      result.push_back(inputs_[i]);
    }
  }

  return result;
}

size_t Node::num_value_inputs() const { return value_inputs().size(); }

Node* Node::region_input() const {
  // Valid only for Phi nodes (S2)
  if (opcode_ == Opcode::kPhi && num_inputs() > 0) {
    return input(0);
  }
  return nullptr;
}

std::vector<Node*> Node::phi_values() const {
  std::vector<Node*> result;
  // Valid only for Phi nodes (S2)
  if (opcode_ == Opcode::kPhi) {
    for (size_t i = 1; i < num_inputs(); ++i) {
      if (inputs_[i] != nullptr) {
        result.push_back(inputs_[i]);
      }
    }
  }
  return result;
}

std::vector<Node*> Node::region_preds() const {
  std::vector<Node*> result;
  // Valid only for Region nodes (S2)
  if (opcode_ == Opcode::kRegion || opcode_ == Opcode::kMergeMem) {
    for (size_t i = 0; i < num_inputs(); ++i) {
      if (inputs_[i] != nullptr) {
        result.push_back(inputs_[i]);
      }
    }
  }
  return result;
}

Node* Node::address_input() const {
  auto s = schema();
  switch (s) {
    case NodeSchema::kS3_Load:
    case NodeSchema::kS4_Store:
      // Address is at index 2 for Load and Store
      if (num_inputs() > 2) {
        return input(2);
      }
      return nullptr;
    default:
      return nullptr;
  }
}

Node* Node::store_value_input() const {
  // Valid only for Store nodes (S4)
  if (schema() == NodeSchema::kS4_Store && num_inputs() > 3) {
    return input(3);
  }
  return nullptr;
}

bool Node::ValidateInputs() const {
  auto s = schema();

  switch (s) {
    case NodeSchema::kS0_Pure:
      // Pure nodes can have any number of inputs (constants have 0, binary ops
      // have 2, etc.)
      return true;

    case NodeSchema::kS1_Control:
      // Control nodes must have at least a control input
      // Some (like If) also need a condition, but we're lenient here
      return num_inputs() >= 1;

    case NodeSchema::kS2_Merge:
      // Phi must have at least Region + one value
      if (opcode_ == Opcode::kPhi) {
        return num_inputs() >= 2;
      }
      // Region/MergeMem must have at least one predecessor
      return num_inputs() >= 1;

    case NodeSchema::kS3_Load:
      // Load must have: control, memory, address (minimum 3)
      if (num_inputs() < 3) {
        return false;
      }
      // Optional: validate that input(0) is a control node
      // For now, just check count
      return true;

    case NodeSchema::kS4_Store:
      // Store must have: control, memory, address, value (minimum 4)
      if (num_inputs() < 4) {
        return false;
      }
      return true;

    case NodeSchema::kS5_Allocate:
      // Allocate must have: control, memory (minimum 2)
      if (num_inputs() < 2) {
        return false;
      }
      return true;

    case NodeSchema::kS6_Return:
      // Return must have at least control
      return num_inputs() >= 1;

    case NodeSchema::kS7_Start:
      // Start has no inputs
      return true;

    case NodeSchema::kS8_Projection:
      // Proj must have a source node
      return num_inputs() >= 1;

    case NodeSchema::kS9_Parameter:
      // Parm must have Start node as input
      return num_inputs() >= 1;

    case NodeSchema::kUnknown:
    default:
      // Don't validate unknown schemas
      return true;
  }
}

}  // namespace sun
