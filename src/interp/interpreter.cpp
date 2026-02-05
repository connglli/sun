#include "suntv/interp/interpreter.hpp"

#include <algorithm>
#include <queue>
#include <set>

#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

// Helper functions to categorize opcodes
static bool IsArithmetic(Opcode op) {
  return op == Opcode::kAddI || op == Opcode::kSubI || op == Opcode::kMulI ||
         op == Opcode::kDivI || op == Opcode::kModI || op == Opcode::kAbsI ||
         op == Opcode::kAddL || op == Opcode::kSubL || op == Opcode::kMulL ||
         op == Opcode::kDivL || op == Opcode::kModL || op == Opcode::kAbsL ||
         op == Opcode::kConvI2L || op == Opcode::kConvL2I;
}

static bool IsBitwise(Opcode op) {
  return op == Opcode::kAndI || op == Opcode::kOrI || op == Opcode::kXorI ||
         op == Opcode::kLShiftI || op == Opcode::kRShiftI ||
         op == Opcode::kURShiftI || op == Opcode::kAndL || op == Opcode::kOrL ||
         op == Opcode::kXorL || op == Opcode::kLShiftL ||
         op == Opcode::kRShiftL || op == Opcode::kURShiftL;
}

static bool IsComparison(Opcode op) {
  return op == Opcode::kCmpI || op == Opcode::kCmpL || op == Opcode::kCmpP ||
         op == Opcode::kCmpU || op == Opcode::kCmpUL;
}

Interpreter::Interpreter(const Graph& g) : graph_(g) {}

Outcome Interpreter::Execute(const std::vector<Value>& inputs) {
  value_cache_.clear();
  region_predecessor_.clear();
  loop_iterations_.clear();
  heap_ = ConcreteHeap();  // Reset heap

  // Cache parameter values first
  // Get all parameter nodes and filter to data parameters only
  std::vector<Node*> all_params = graph_.GetParameterNodes();
  std::vector<Node*> params;

  for (Node* p : all_params) {
    // Only include data parameters (skip control, memory, I/O, etc.)
    // Data parameters have types like "int:", "long:", etc.
    if (p->has_prop("type")) {
      std::string type = std::get<std::string>(p->prop("type"));
      // Skip non-data types
      if (type == "control" || type == "memory" || type == "return_address" ||
          type == "rawptr:" || type == "abIO") {
        continue;
      }
      // Only include types that end with ':' (int:, long:, etc.)
      if (type.empty() || type.back() != ':') {
        continue;
      }
    }
    params.push_back(p);
  }

  // Sort parameters by their index
  std::sort(params.begin(), params.end(), [](Node* a, Node* b) {
    // Try to get index from property first
    if (a->has_prop("index") && b->has_prop("index")) {
      return std::get<int32_t>(a->prop("index")) <
             std::get<int32_t>(b->prop("index"));
    }

    // Try to extract from dump_spec (e.g., "Parm0: int")
    auto get_index = [](Node* n) -> int32_t {
      if (n->has_prop("dump_spec")) {
        std::string spec = std::get<std::string>(n->prop("dump_spec"));
        size_t parm_pos = spec.find("Parm");
        if (parm_pos != std::string::npos) {
          size_t colon_pos = spec.find(':', parm_pos);
          if (colon_pos != std::string::npos) {
            std::string num_str =
                spec.substr(parm_pos + 4, colon_pos - parm_pos - 4);
            try {
              return std::stoi(num_str);
            } catch (...) {
              return 999999;  // Invalid index
            }
          }
        }
      }
      return 999999;  // No index found
    };

    return get_index(a) < get_index(b);
  });

  for (Node* parm_node : params) {
    Value parm_val = EvalParm(parm_node, inputs);
    value_cache_[parm_node] = parm_val;
  }

  // Start control flow traversal from Start node
  Node* start = graph_.start();
  if (!start) {
    throw std::runtime_error("No Start node found in graph");
  }

  // Traverse control flow until we reach Return
  const Node* current_control = start;
  while (current_control && current_control->opcode() != Opcode::kReturn) {
    current_control = StepControl(current_control);
  }

  if (!current_control) {
    throw std::runtime_error("Control flow terminated without reaching Return");
  }

  // Evaluate the return value (if any)
  Outcome outcome;
  outcome.kind = Outcome::Kind::kReturn;

  // C2 Return node structure:
  // input[0] = control
  // input[1..n-1] = various (memory, frame pointer, etc.)
  // Last input = return value (if method returns non-void)
  // Find the return value: it's typically the last input that's not a Parm
  Node* value_node = nullptr;
  for (int i = current_control->num_inputs() - 1; i >= 1; --i) {
    Node* inp = current_control->input(i);
    if (inp && inp->opcode() != Opcode::kParm) {
      value_node = inp;
      break;
    }
  }

  if (value_node) {
    try {
      Value result = EvalNode(value_node);
      outcome.return_value = result;
    } catch (const EvalException& e) {
      // Convert evaluation exception to Throw outcome
      outcome.kind = Outcome::Kind::kThrow;
      outcome.exception_kind = e.what();
    }
  }

  outcome.heap = heap_;
  return outcome;
}

const Node* Interpreter::StepControl(const Node* ctrl) {
  if (!ctrl) {
    throw std::runtime_error("StepControl called with null control node");
  }

  Opcode op = ctrl->opcode();
  Logger::Debug("StepControl: node " + std::to_string(ctrl->id()) + " (" +
                OpcodeToString(op) + ")");

  switch (op) {
    case Opcode::kStart:
    case Opcode::kGoto:
    case Opcode::kIfTrue:
    case Opcode::kIfFalse:
    case Opcode::kParm:  // Parm nodes act as control projections in C2
      // Simple pass-through: find successor
      return FindControlSuccessor(ctrl);

    case Opcode::kIf: {
      // Evaluate condition and choose branch
      if (ctrl->num_inputs() < 2) {
        throw std::runtime_error("If node needs condition input");
      }

      // Evaluate condition (this may recursively evaluate data subgraph)
      Value cond = EvalNode(ctrl->input(1));

      bool branch_taken = false;
      if (cond.is_bool()) {
        branch_taken = cond.as_bool();
      } else if (cond.is_i32()) {
        // C2 sometimes uses int as bool (non-zero = true)
        branch_taken = (cond.as_i32() != 0);
      } else {
        throw std::runtime_error("If condition must be boolean or int");
      }

      Logger::Debug("  If condition evaluated to: " +
                    std::string(branch_taken ? "true" : "false"));

      // Find the corresponding IfTrue or IfFalse successor
      for (Node* n : graph_.nodes()) {
        if (n->num_inputs() > 0 && n->input(0) == ctrl) {
          if (branch_taken && n->opcode() == Opcode::kIfTrue) {
            return n;
          } else if (!branch_taken && n->opcode() == Opcode::kIfFalse) {
            return n;
          }
        }
      }

      throw std::runtime_error("If node has no IfTrue/IfFalse successors");
    }

    case Opcode::kRegion: {
      // Control merge point
      // Check if this is a loop header
      if (IsLoopHeader(ctrl)) {
        // Determine which input we came from
        const Node* predecessor = region_predecessor_.count(ctrl) > 0
                                      ? region_predecessor_[ctrl]
                                      : nullptr;

        // Only count iterations when we arrive via the back-edge
        // For a self-loop (input[0] == ctrl), the back-edge is when
        // predecessor == ctrl (coming from the Region itself)
        bool is_back_edge = (predecessor == ctrl);

        if (is_back_edge) {
          // Loop iteration: check iteration count
          int iter_count = loop_iterations_[ctrl];
          if (iter_count >= kMaxLoopIterations) {
            throw std::runtime_error("Loop exceeded maximum iterations (" +
                                     std::to_string(kMaxLoopIterations) + ")");
          }
          loop_iterations_[ctrl] = iter_count + 1;

          Logger::Debug("  Loop iteration " + std::to_string(iter_count + 1));
        }
      }

      // Continue to successor
      return FindControlSuccessor(ctrl);
    }

    default:
      throw std::runtime_error("Unexpected control opcode in StepControl: " +
                               OpcodeToString(op));
  }
}

const Node* Interpreter::FindControlSuccessor(const Node* ctrl) const {
  // Find the node that uses ctrl as its control input
  // Most nodes use input[0] for control

  for (Node* n : graph_.nodes()) {
    if (n->num_inputs() > 0 && n->input(0) == ctrl) {
      // Check if this is a control node
      Opcode op = n->opcode();
      if (op == Opcode::kIf || op == Opcode::kIfTrue ||
          op == Opcode::kIfFalse || op == Opcode::kRegion ||
          op == Opcode::kGoto || op == Opcode::kReturn) {
        // Before returning, if this is a Region, record where we came from
        if (op == Opcode::kRegion) {
          // We need to record which input of the Region corresponds to ctrl
          // This is used by Phi nodes
          const_cast<Interpreter*>(this)->region_predecessor_[n] = ctrl;
        }
        return n;
      }

      // Parm can be control projection in C2, but only if type="control"
      if (op == Opcode::kParm) {
        if (n->has_prop("type")) {
          std::string type = std::get<std::string>(n->prop("type"));
          if (type == "control") {
            return n;
          }
        }
      }
    }

    // For Region nodes, check all inputs (not just input[0])
    if (n->opcode() == Opcode::kRegion) {
      for (size_t i = 0; i < n->num_inputs(); ++i) {
        if (n->input(i) == ctrl) {
          // Record predecessor
          const_cast<Interpreter*>(this)->region_predecessor_[n] = ctrl;
          return n;
        }
      }
    }
  }

  return nullptr;  // No successor (end of control flow)
}

bool Interpreter::IsLoopHeader(const Node* region) const {
  if (!region || region->opcode() != Opcode::kRegion) {
    return false;
  }

  // A Region is a loop header if it has a back-edge: one of its inputs
  // is a control node that is reachable from the Region itself

  // Simple heuristic: check if Region input[0] points to itself (self-loop)
  if (region->num_inputs() > 0 && region->input(0) == region) {
    return true;
  }

  // More complex: check if any input is "downstream" from the Region
  // For now, we'll use the simple heuristic
  // TODO: Implement full back-edge detection if needed

  return false;
}

Value Interpreter::EvalNode(const Node* n) {
  if (!n) {
    throw std::runtime_error("EvalNode called with null node");
  }

  // Check cache first
  auto it = value_cache_.find(n);
  if (it != value_cache_.end()) {
    return it->second;
  }

  std::cerr << "EvalNode: " << n->id() << " (" << OpcodeToString(n->opcode())
            << ")\n";

  Value result;
  Opcode op = n->opcode();

  // Dispatch based on opcode
  if (op == Opcode::kConI || op == Opcode::kConL || op == Opcode::kConP) {
    result = EvalConst(n);
  } else if (op == Opcode::kParm) {
    // Parm nodes should have been cached during Execute()
    // If we reach here, it means we're evaluating a Parm that wasn't cached
    // (e.g., a non-data Parm like control or memory)
    // Return a dummy value
    return Value::MakeI32(0);
  } else if (op == Opcode::kBool) {
    result = EvalBool(n);
  } else if (op == Opcode::kConv2B) {
    result = EvalConv2B(n);
  } else if (op == Opcode::kCMoveI || op == Opcode::kCMoveL ||
             op == Opcode::kCMoveP) {
    result = EvalCMove(n);
  } else if (op == Opcode::kPhi) {
    result = EvalPhi(n);
  } else if (op == Opcode::kSafePoint || op == Opcode::kOpaque1 ||
             op == Opcode::kParsePredicate) {
    result = EvalNoOp(n);
  } else if (op == Opcode::kThreadLocal) {
    result = EvalThreadLocal(n);
  } else if (op == Opcode::kCallStaticJava) {
    result = EvalCallStaticJava(n);
  } else if (op == Opcode::kHalt) {
    result = EvalHalt(n);
  } else if (op == Opcode::kAllocate) {
    result = EvalAllocate(n);
  } else if (op == Opcode::kAllocateArray) {
    result = EvalAllocateArray(n);
  } else if (op == Opcode::kLoadB || op == Opcode::kLoadUB ||
             op == Opcode::kLoadS || op == Opcode::kLoadUS ||
             op == Opcode::kLoadI || op == Opcode::kLoadL ||
             op == Opcode::kLoadP || op == Opcode::kLoadN) {
    result = EvalLoad(n);
  } else if (IsArithmetic(op) || IsBitwise(op)) {
    result = EvalArithOp(n);
  } else if (IsComparison(op)) {
    result = EvalCmpOp(n);
  } else {
    throw std::runtime_error("Unsupported opcode: " +
                             OpcodeToString(n->opcode()));
  }

  // Cache the result
  value_cache_[n] = result;
  return result;
}

Value Interpreter::EvalConst(const Node* n) {
  Opcode op = n->opcode();

  if (op == Opcode::kConI) {
    // Try 'value' property first (manually constructed graphs)
    if (n->has_prop("value")) {
      int32_t val = std::get<int32_t>(n->prop("value"));
      return Value::MakeI32(val);
    }
    // C2 graphs encode value in dump_spec: " #int:42" or " #int:-5"
    if (n->has_prop("dump_spec")) {
      std::string spec = std::get<std::string>(n->prop("dump_spec"));
      // Format: " #int:<value>"
      size_t colon = spec.find(':');
      if (colon != std::string::npos) {
        std::string val_str = spec.substr(colon + 1);
        int32_t val = std::stoi(val_str);
        return Value::MakeI32(val);
      }
    }
    throw std::runtime_error(
        "ConI node missing 'value' or parseable 'dump_spec' property");
  } else if (op == Opcode::kConL) {
    // Try 'value' property first (manually constructed graphs)
    if (n->has_prop("value")) {
      int64_t val = std::get<int64_t>(n->prop("value"));
      return Value::MakeI64(val);
    }
    // C2 graphs encode value in dump_spec: " #long:42" or " #long:-5"
    if (n->has_prop("dump_spec")) {
      std::string spec = std::get<std::string>(n->prop("dump_spec"));
      // Format: " #long:<value>"
      size_t colon = spec.find(':');
      if (colon != std::string::npos) {
        std::string val_str = spec.substr(colon + 1);
        int64_t val = std::stoll(val_str);
        return Value::MakeI64(val);
      }
    }
    throw std::runtime_error(
        "ConL node missing 'value' or parseable 'dump_spec' property");
  } else if (op == Opcode::kConP) {
    // Null pointer constant
    return Value::MakeNull();
  }

  throw std::runtime_error("Unknown constant opcode");
}

Value Interpreter::EvalParm(const Node* n, const std::vector<Value>& inputs) {
  // Try to get index from property (for manually constructed graphs)
  if (n->has_prop("index")) {
    int32_t index = std::get<int32_t>(n->prop("index"));
    if (index < 0 || index >= static_cast<int32_t>(inputs.size())) {
      throw std::runtime_error("Parm index out of range");
    }
    return inputs[index];
  }

  // For C2 graphs: Extract index from dump_spec (e.g., "Parm0: int")
  if (n->has_prop("dump_spec")) {
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    // Look for "Parm<N>:" pattern
    size_t parm_pos = spec.find("Parm");
    if (parm_pos != std::string::npos) {
      size_t colon_pos = spec.find(':', parm_pos);
      if (colon_pos != std::string::npos) {
        std::string num_str =
            spec.substr(parm_pos + 4, colon_pos - parm_pos - 4);
        try {
          int32_t index = std::stoi(num_str);
          if (index < 0 || index >= static_cast<int32_t>(inputs.size())) {
            throw std::runtime_error("Parm index out of range: " +
                                     std::to_string(index));
          }
          return inputs[index];
        } catch (const std::exception& e) {
          throw std::runtime_error(
              "Failed to parse Parm index from dump_spec: " + spec);
        }
      }
    }
  }

  // Fallback: non-data Parm nodes (control, memory, etc.) return dummy value
  // Check if this is a data parameter by looking at the 'type' property
  if (n->has_prop("type")) {
    std::string type = std::get<std::string>(n->prop("type"));
    if (type == "control" || type == "memory" || type == "return_address") {
      // Not a data parameter, return dummy
      return Value::MakeI32(0);
    }
  }

  throw std::runtime_error("Parm node missing 'index' or 'dump_spec' property");
}

Value Interpreter::EvalArithOp(const Node* n) {
  Opcode op = n->opcode();

  // Handle unary operations (AbsI, AbsL, conversions)
  if (op == Opcode::kAbsI || op == Opcode::kAbsL || op == Opcode::kConvI2L ||
      op == Opcode::kConvL2I) {
    std::cerr << "EvalArithOp unary: node has " << n->num_inputs()
              << " inputs\n";
    for (size_t i = 0; i < n->num_inputs(); i++) {
      std::cerr << "  input[" << i << "] = "
                << (n->input(i) ? OpcodeToString(n->input(i)->opcode())
                                : "nullptr")
                << "\n";
    }

    // C2 format check: if input[0] is null, try input[1]
    const Node* operand = nullptr;
    if (n->num_inputs() >= 2 && n->input(0) == nullptr) {
      operand = n->input(1);
      std::cerr << "  Using input[1] (C2 format)\n";
    } else if (n->num_inputs() >= 1) {
      operand = n->input(0);
      std::cerr << "  Using input[0] (old format)\n";
    } else {
      throw std::runtime_error("Unary op needs at least 1 input");
    }

    Value a = EvalNode(operand);

    switch (op) {
      case Opcode::kAbsI:
        return Evaluator::EvalAbsI(a);
      case Opcode::kAbsL:
        return Evaluator::EvalAbsL(a);
      case Opcode::kConvI2L:
        return Evaluator::EvalConvI2L(a);
      case Opcode::kConvL2I:
        return Evaluator::EvalConvL2I(a);
      default:
        throw std::runtime_error("Unsupported unary opcode");
    }
  }

  // Binary operations
  if (n->num_inputs() < 2) {
    throw std::runtime_error("Binary op needs at least 2 inputs");
  }

  Value a = EvalNode(n->input(0));
  Value b = EvalNode(n->input(1));

  switch (op) {
    // Int32 arithmetic
    case Opcode::kAddI:
      return Evaluator::EvalAddI(a, b);
    case Opcode::kSubI:
      return Evaluator::EvalSubI(a, b);
    case Opcode::kMulI:
      return Evaluator::EvalMulI(a, b);
    case Opcode::kDivI:
      return Evaluator::EvalDivI(a, b);
    case Opcode::kModI:
      return Evaluator::EvalModI(a, b);

    // Int64 arithmetic
    case Opcode::kAddL:
      return Evaluator::EvalAddL(a, b);
    case Opcode::kSubL:
      return Evaluator::EvalSubL(a, b);
    case Opcode::kMulL:
      return Evaluator::EvalMulL(a, b);
    case Opcode::kDivL:
      return Evaluator::EvalDivL(a, b);
    case Opcode::kModL:
      return Evaluator::EvalModL(a, b);

    // Int32 bitwise
    case Opcode::kAndI:
      return Evaluator::EvalAndI(a, b);
    case Opcode::kOrI:
      return Evaluator::EvalOrI(a, b);
    case Opcode::kXorI:
      return Evaluator::EvalXorI(a, b);
    case Opcode::kLShiftI:
      return Evaluator::EvalLShiftI(a, b);
    case Opcode::kRShiftI:
      return Evaluator::EvalRShiftI(a, b);
    case Opcode::kURShiftI:
      return Evaluator::EvalURShiftI(a, b);

    // Int64 bitwise
    case Opcode::kAndL:
      return Evaluator::EvalAndL(a, b);
    case Opcode::kOrL:
      return Evaluator::EvalOrL(a, b);
    case Opcode::kXorL:
      return Evaluator::EvalXorL(a, b);
    case Opcode::kLShiftL:
      return Evaluator::EvalLShiftL(a, b);
    case Opcode::kRShiftL:
      return Evaluator::EvalRShiftL(a, b);
    case Opcode::kURShiftL:
      return Evaluator::EvalURShiftL(a, b);

    default:
      throw std::runtime_error("Unsupported arithmetic opcode");
  }
}

Value Interpreter::EvalCmpOp(const Node* n) {
  // Support both old format and C2 format:
  // Old format (manually constructed): input[0] = first operand, input[1] =
  // second operand C2 format: input[0] = unused/region, input[1] = first
  // operand, input[2] = second operand

  Value a, b;

  if (n->num_inputs() >= 3 && n->input(0) == nullptr) {
    // C2 format: inputs at [1] and [2]
    a = EvalNode(n->input(1));
    b = EvalNode(n->input(2));
  } else if (n->num_inputs() >= 2) {
    // Old format: inputs at [0] and [1]
    a = EvalNode(n->input(0));
    b = EvalNode(n->input(1));
  } else {
    throw std::runtime_error("Comparison op needs at least 2 inputs");
  }

  Opcode op = n->opcode();

  // CmpI/CmpL/CmpP return tri-state: -1 (less), 0 (equal), 1 (greater)
  switch (op) {
    case Opcode::kCmpI: {
      int32_t av = a.as_i32();
      int32_t bv = b.as_i32();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpL: {
      int64_t av = a.as_i64();
      int64_t bv = b.as_i64();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    case Opcode::kCmpP: {
      // For pointers, compare refs
      if (!a.is_ref() && !a.is_null()) {
        throw std::runtime_error("CmpP expects ref or null for first operand");
      }
      if (!b.is_ref() && !b.is_null()) {
        throw std::runtime_error("CmpP expects ref or null for second operand");
      }
      // Compare: null < ref, refs by numeric value
      int32_t av = a.is_null() ? 0 : a.as_ref();
      int32_t bv = b.is_null() ? 0 : b.as_ref();
      if (av < bv)
        return Value::MakeI32(-1);
      else if (av > bv)
        return Value::MakeI32(1);
      else
        return Value::MakeI32(0);
    }
    default:
      throw std::runtime_error("Unsupported comparison opcode");
  }
}

Value Interpreter::EvalPhi(const Node* n) {
  // Phi selects value based on which control predecessor was taken
  // Phi structure: input(0) = Region control, input(1..k) = values
  if (n->num_inputs() < 2) {
    throw std::runtime_error("Phi node needs at least control + 1 value");
  }

  Node* region = n->input(0);
  if (!region || region->opcode() != Opcode::kRegion) {
    // Simplified case: no region, just take first value
    return EvalNode(n->input(1));
  }

  // Determine which control predecessor was taken
  auto it = region_predecessor_.find(region);
  if (it == region_predecessor_.end()) {
    // No predecessor recorded - this shouldn't happen in proper execution
    // Fall back to first value
    Logger::Warn("Phi node " + std::to_string(n->id()) +
                 ": no predecessor recorded for Region " +
                 std::to_string(region->id()) + ", using first value");
    return EvalNode(n->input(1));
  }

  const Node* active_pred = it->second;
  Logger::Debug("EvalPhi: Region " + std::to_string(region->id()) +
                " active predecessor = " + std::to_string(active_pred->id()));

  // Find which Region input corresponds to active_pred
  // Note: Region may have self-loops (input[0] -> Region itself for loops)
  // We need to map Region input index to Phi input index
  // Skipping self-loops

  int phi_input_index = 1;  // Phi inputs start at index 1
  for (size_t i = 0; i < region->num_inputs(); ++i) {
    Node* reg_input = region->input(i);

    // Skip self-loops
    if (reg_input == region) {
      continue;
    }

    // Check if this is the active predecessor
    if (reg_input == active_pred) {
      // Found it! Select corresponding Phi input
      if (phi_input_index < static_cast<int>(n->num_inputs())) {
        Value result = EvalNode(n->input(phi_input_index));
        Logger::Debug("  Phi selected input[" +
                      std::to_string(phi_input_index) + "]");
        return result;
      } else {
        throw std::runtime_error("Phi node: input index out of range");
      }
    }

    phi_input_index++;
  }

  // Predecessor not found - shouldn't happen
  throw std::runtime_error(
      "Phi node: active predecessor not found in Region inputs");
}

Value Interpreter::EvalBool(const Node* n) {
  // Bool node converts comparison result to boolean

  std::cerr << "EvalBool: node " << n->id() << " has " << n->num_inputs()
            << " inputs\n";
  for (size_t i = 0; i < n->num_inputs(); ++i) {
    Node* inp = n->input(i);
    if (inp) {
      std::cerr << "  input[" << i << "] = Node " << inp->id() << " ("
                << OpcodeToString(inp->opcode()) << ")\n";
    } else {
      std::cerr << "  input[" << i << "] = nullptr\n";
    }
  }

  if (n->num_inputs() < 1) {
    throw std::runtime_error("Bool node needs comparison input");
  }

  // Support both old format and C2 format:
  // Old format (manually constructed): input[0] = comparison
  // C2 format: input[0] = unused/nullptr, input[1] = comparison
  Node* cmp_node = nullptr;
  if (n->num_inputs() >= 2 && n->input(0) == nullptr) {
    // C2 format: comparison at input[1]
    cmp_node = n->input(1);
  } else {
    // Old format: comparison at input[0]
    cmp_node = n->input(0);
  }

  if (!cmp_node) {
    throw std::runtime_error("Bool node comparison input is null");
  }

  Value cmp_result = EvalNode(cmp_node);
  if (!cmp_result.is_i32()) {
    throw std::runtime_error("Bool node expects i32 comparison result");
  }

  int32_t cmp_val = cmp_result.as_i32();
  std::cerr << "EvalBool: cmp_val = " << cmp_val << "\n";

  // Get mask property (condition code)
  int32_t mask = 0;
  if (n->has_prop("mask")) {
    mask = std::get<int32_t>(n->prop("mask"));
    std::cerr << "EvalBool: mask = " << mask << "\n";
  } else if (n->has_prop("dump_spec")) {
    // Parse dump_spec for condition (e.g., "[le]")
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    std::cerr << "EvalBool: dump_spec = " << spec << "\n";
    // Map dump_spec to mask
    // le = LT|EQ = 1|2 = 3, gt = 4, ge = GT|EQ = 4|2 = 6, etc.
    if (spec.find("le") != std::string::npos) {
      mask = 3;  // LT | EQ
    } else if (spec.find("lt") != std::string::npos) {
      mask = 1;  // LT
    } else if (spec.find("ge") != std::string::npos) {
      mask = 6;  // GT | EQ
    } else if (spec.find("gt") != std::string::npos) {
      mask = 4;  // GT
    } else if (spec.find("eq") != std::string::npos) {
      mask = 2;  // EQ
    } else if (spec.find("ne") != std::string::npos) {
      mask = 5;  // LT | GT
    }
    std::cerr << "EvalBool: parsed mask = " << mask << "\n";
  }

  // HotSpot condition codes (bit encoding):
  // Bit 0 (value 1): LT (less than)
  // Bit 1 (value 2): EQ (equal)
  // Bit 2 (value 4): GT (greater than)
  // Examples: NE = LT|GT = 1|4 = 5, LE = LT|EQ = 1|2 = 3, GE = GT|EQ = 4|2 = 6
  // Comparison returns: -1 (less), 0 (equal), 1 (greater)
  bool result = false;
  if (cmp_val < 0 && (mask & 1))
    result = true;  // LT
  else if (cmp_val == 0 && (mask & 2))
    result = true;  // EQ
  else if (cmp_val > 0 && (mask & 4))
    result = true;  // GT

  std::cerr << "EvalBool: result = " << result << "\n";
  return Value::MakeBool(result);
}

Value Interpreter::EvalCMove(const Node* n) {
  // CMoveI/L/P: conditional move
  // Input(0) = condition (boolean)
  // Input(1) = value if true
  // Input(2) = value if false
  if (n->num_inputs() < 3) {
    throw std::runtime_error("CMove needs 3 inputs");
  }

  Value cond = EvalNode(n->input(0));
  if (!cond.is_bool()) {
    throw std::runtime_error("CMove condition must be boolean");
  }

  if (cond.as_bool()) {
    return EvalNode(n->input(1));
  } else {
    return EvalNode(n->input(2));
  }
}

Value Interpreter::EvalConv2B(const Node* n) {
  // Conv2B: Convert any value to boolean (0 -> 0, non-zero -> 1)
  // Input(0) = value to convert
  if (n->num_inputs() < 1) {
    throw std::runtime_error("Conv2B needs input value");
  }

  Value input = EvalNode(n->input(0));

  // Convert to boolean: 0 -> 0, non-zero -> 1
  if (input.is_i32()) {
    return Value::MakeI32(input.as_i32() != 0 ? 1 : 0);
  } else if (input.is_i64()) {
    return Value::MakeI32(input.as_i64() != 0 ? 1 : 0);
  } else if (input.is_ref()) {
    return Value::MakeI32(input.as_ref() != 0 ? 1 : 0);
  } else if (input.is_null()) {
    return Value::MakeI32(0);
  } else if (input.is_bool()) {
    return Value::MakeI32(input.as_bool() ? 1 : 0);
  } else {
    throw std::runtime_error("Conv2B: unsupported input type");
  }
}

Value Interpreter::EvalNoOp(const Node* n) {
  // SafePoint, Opaque1, ParsePredicate: optimization markers, pass through
  // These nodes typically have a value input that we should pass through
  if (n->num_inputs() > 0) {
    // Pass through the first input (typically the value)
    return EvalNode(n->input(0));
  }
  // If no inputs, return a dummy value (shouldn't happen in practice)
  return Value::MakeI32(0);
}

Value Interpreter::EvalThreadLocal(const Node* /*n*/) {
  // ThreadLocal: thread-local variable access
  // For prototype: return a dummy reference (null)
  // In real implementation, would need to track thread-local storage
  return Value::MakeNull();
}

Value Interpreter::EvalCallStaticJava(const Node* n) {
  // CallStaticJava: static method call
  // Most of these in C2 graphs are uncommon_trap (deopt guards)
  // Check if it's an uncommon_trap and skip it
  if (n->has_prop("dump_spec")) {
    std::string spec = std::get<std::string>(n->prop("dump_spec"));
    if (spec.find("uncommon_trap") != std::string::npos) {
      // Uncommon trap - assume it doesn't fire, return dummy value
      return Value::MakeI32(0);
    }
  }

  // Real method call - not supported in prototype
  throw std::runtime_error(
      "CallStaticJava: real method calls not supported in prototype");
}

Value Interpreter::EvalHalt(const Node* /*n*/) {
  // Halt: abnormal termination (e.g., unhandled exception)
  throw std::runtime_error("Program reached Halt node (abnormal termination)");
}

Value Interpreter::EvalAllocate(const Node* /*n*/) {
  // Allocate object: input(0) = control
  // Returns a fresh reference
  // Note: Control dependency is implicit; concrete execution doesn't check it
  Ref new_ref = heap_.AllocateObject();
  return Value::MakeRef(new_ref);
}

Value Interpreter::EvalAllocateArray(const Node* n) {
  // AllocateArray: input(0) = control, input(1) = length
  if (n->num_inputs() < 2) {
    throw std::runtime_error("AllocateArray needs length input");
  }

  Value len_val = EvalNode(n->input(1));
  if (!len_val.is_i32()) {
    throw std::runtime_error("Array length must be i32");
  }

  int32_t length = len_val.as_i32();
  if (length < 0) {
    throw EvalException("Negative array length");
  }

  Ref arr_ref = heap_.AllocateArray(length);
  return Value::MakeRef(arr_ref);
}

Value Interpreter::EvalLoad(const Node* n) {
  // Load: input(0) = control, input(1) = memory, input(2) = base,
  //       [optional input(3) = index for arrays]
  if (n->num_inputs() < 3) {
    throw std::runtime_error("Load needs at least control, memory, and base");
  }

  // Process memory chain (execute any Store nodes in the chain)
  Node* mem = n->input(1);
  ProcessMemoryChain(mem);

  // Get base object/array
  Value base_val = EvalNode(n->input(2));
  if (!base_val.is_ref()) {
    throw std::runtime_error("Load base must be a reference");
  }
  Ref base = base_val.as_ref();

  // Check if this is array access
  bool is_array = n->has_prop("array") && std::get<bool>(n->prop("array"));

  if (is_array) {
    // Array access: needs index
    if (n->num_inputs() < 4) {
      throw std::runtime_error("Array load needs index");
    }
    Value idx_val = EvalNode(n->input(3));
    if (!idx_val.is_i32()) {
      throw std::runtime_error("Array index must be i32");
    }
    int32_t index = idx_val.as_i32();

    // Load from array
    Value elem = heap_.ReadArray(base, index);
    return elem;
  } else {
    // Field access
    if (!n->has_prop("field")) {
      throw std::runtime_error("Load needs field property");
    }
    std::string field = std::get<std::string>(n->prop("field"));

    Value field_val = heap_.ReadField(base, field);
    return field_val;
  }
}

void Interpreter::ProcessMemoryChain(const Node* mem) {
  // Process Store nodes in memory chain
  if (!mem) return;

  Opcode op = mem->opcode();

  // If this is a Store, execute it
  if (op == Opcode::kStoreB || op == Opcode::kStoreC || op == Opcode::kStoreI ||
      op == Opcode::kStoreL || op == Opcode::kStoreP || op == Opcode::kStoreN) {
    EvalStore(mem);
  }
  // If Store has a memory input, process that first (in reverse order)
  if (mem->num_inputs() >= 2) {
    ProcessMemoryChain(mem->input(1));
  }
}

void Interpreter::EvalStore(const Node* n) {
  // Store: input(0) = control, input(1) = memory, input(2) = base,
  //        input(3) = value (field) or input(3) = index, input(4) = value
  //        (array)

  if (n->num_inputs() < 4) {
    throw std::runtime_error(
        "Store needs at least control, memory, base, value");
  }

  // Get base object/array
  Value base_val = EvalNode(n->input(2));
  if (!base_val.is_ref()) {
    throw std::runtime_error("Store base must be a reference");
  }
  Ref base = base_val.as_ref();

  // Check if this is array access
  bool is_array = n->has_prop("array") && std::get<bool>(n->prop("array"));

  if (is_array) {
    // Array access: input(3) = index, input(4) = value
    if (n->num_inputs() < 5) {
      throw std::runtime_error("Array store needs index and value");
    }

    Value idx_val = EvalNode(n->input(3));
    if (!idx_val.is_i32()) {
      throw std::runtime_error("Array index must be i32");
    }
    int32_t index = idx_val.as_i32();

    Value value = EvalNode(n->input(4));
    heap_.WriteArray(base, index, value);
  } else {
    // Field access: input(3) = value
    if (!n->has_prop("field")) {
      throw std::runtime_error("Store needs field property");
    }
    std::string field = std::get<std::string>(n->prop("field"));

    Value value = EvalNode(n->input(3));
    heap_.WriteField(base, field, value);
  }
}

}  // namespace sun
