#include "suntv/interp/interpreter.hpp"

#include <algorithm>

#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

// Helper functions to categorize opcodes
static bool IsArithmetic(Opcode op) {
  return op == Opcode::kAddI || op == Opcode::kSubI || op == Opcode::kMulI ||
         op == Opcode::kDivI || op == Opcode::kModI || op == Opcode::kAddL ||
         op == Opcode::kSubL || op == Opcode::kMulL || op == Opcode::kDivL ||
         op == Opcode::kModL || op == Opcode::kConvI2L ||
         op == Opcode::kConvL2I;
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
  control_active_.clear();
  if_decisions_.clear();
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

  // Compute control flow starting from Start node
  Node* start = graph_.start();
  if (start) {
    ComputeControlFlow(start);
  }

  // Find Return node
  // In IGV graphs, Root exists but doesn't point to Return
  // We need to search for the Return node directly
  Node* ret_node = nullptr;
  for (Node* n : graph_.nodes()) {
    if (n->opcode() == Opcode::kReturn) {
      ret_node = n;
      break;
    }
  }

  if (!ret_node) {
    throw std::runtime_error("No Return node found in graph");
  }

  // Evaluate the return value (if any)
  Outcome outcome;
  outcome.kind = Outcome::Kind::kReturn;

  // Return node has: control (input 0), optional value (input 1)
  if (ret_node->num_inputs() > 1) {
    Node* value_node = ret_node->input(1);
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

Value Interpreter::EvalNode(const Node* n) {
  // Check cache
  auto it = value_cache_.find(n);
  if (it != value_cache_.end()) {
    return it->second;
  }

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

  // Cache result
  value_cache_[n] = result;
  return result;
}

Value Interpreter::EvalConst(const Node* n) {
  Opcode op = n->opcode();

  if (op == Opcode::kConI) {
    if (!n->has_prop("value")) {
      throw std::runtime_error("ConI node missing 'value' property");
    }
    int32_t val = std::get<int32_t>(n->prop("value"));
    return Value::MakeI32(val);
  } else if (op == Opcode::kConL) {
    if (!n->has_prop("value")) {
      throw std::runtime_error("ConL node missing 'value' property");
    }
    int64_t val = std::get<int64_t>(n->prop("value"));
    return Value::MakeI64(val);
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

  // Binary operations
  if (n->num_inputs() < 2) {
    throw std::runtime_error("Arithmetic op needs at least 2 inputs");
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

    // Conversions
    case Opcode::kConvI2L:
      return Evaluator::EvalConvI2L(a);
    case Opcode::kConvL2I:
      return Evaluator::EvalConvL2I(a);

    default:
      throw std::runtime_error("Unsupported arithmetic opcode");
  }
}

Value Interpreter::EvalCmpOp(const Node* n) {
  if (n->num_inputs() < 2) {
    throw std::runtime_error("Comparison op needs 2 inputs");
  }

  Value a = EvalNode(n->input(0));
  Value b = EvalNode(n->input(1));

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
  // Phi selects value based on which control predecessor is active
  // Phi structure: input(0) = Region control, input(1..k) = values
  if (n->num_inputs() < 2) {
    throw std::runtime_error("Phi node needs at least control + 1 value");
  }

  Node* region = n->input(0);
  if (!region || region->opcode() != Opcode::kRegion) {
    // Simplified case: no region, just take first value
    return EvalNode(n->input(1));
  }

  // Find which Region predecessor is active
  for (size_t i = 0; i < region->num_inputs(); ++i) {
    Node* pred_ctrl = region->input(i);
    if (IsControlActive(pred_ctrl)) {
      // Select corresponding value (input index = i + 1)
      if (i + 1 < n->num_inputs()) {
        return EvalNode(n->input(i + 1));
      }
    }
  }

  // Fallback: return first value
  return EvalNode(n->input(1));
}

Value Interpreter::EvalBool(const Node* n) {
  // Bool node converts comparison result to boolean
  // Input(0) = comparison result (CmpI/CmpL/CmpP returns i32)
  // Property "mask" = condition code (e.g., 4 = GT, 2 = LT, 1 = EQ, etc.)
  if (n->num_inputs() < 1) {
    throw std::runtime_error("Bool node needs comparison input");
  }

  Value cmp_result = EvalNode(n->input(0));
  if (!cmp_result.is_i32()) {
    throw std::runtime_error("Bool node expects i32 comparison result");
  }

  int32_t cmp_val = cmp_result.as_i32();

  // Get mask property (condition code)
  int32_t mask = 0;
  if (n->has_prop("mask")) {
    mask = std::get<int32_t>(n->prop("mask"));
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

void Interpreter::ComputeControlFlow(const Node* start_node) {
  // Mark Start as active
  control_active_[start_node] = true;

  // Recursively compute control flow for the graph
  // We need to process control nodes in a specific order
  // For simplicity, we'll do a forward traversal

  // Process all nodes to find control flow
  for (Node* n : graph_.nodes()) {
    Opcode op = n->opcode();

    if (op == Opcode::kStart) {
      control_active_[n] = true;
    } else if (op == Opcode::kIf) {
      // If node: evaluate condition and mark branches
      if (n->num_inputs() >= 2) {
        Node* pred_ctrl = n->input(0);
        if (IsControlActive(pred_ctrl)) {
          // Evaluate condition
          Value cond = EvalNode(n->input(1));
          bool branch_taken = false;
          if (cond.is_bool()) {
            branch_taken = cond.as_bool();
          } else {
            throw std::runtime_error("If condition must be boolean");
          }
          // Store decision
          if_decisions_[n] = branch_taken;
          control_active_[n] = true;
        }
      }
    } else if (op == Opcode::kIfTrue) {
      // IfTrue: active if parent If is active and condition is true
      if (n->num_inputs() >= 1) {
        Node* if_node = n->input(0);
        if (if_node && if_node->opcode() == Opcode::kIf) {
          auto it = if_decisions_.find(if_node);
          if (it != if_decisions_.end() && it->second == true) {
            control_active_[n] = true;
          }
        }
      }
    } else if (op == Opcode::kIfFalse) {
      // IfFalse: active if parent If is active and condition is false
      if (n->num_inputs() >= 1) {
        Node* if_node = n->input(0);
        if (if_node && if_node->opcode() == Opcode::kIf) {
          auto it = if_decisions_.find(if_node);
          if (it != if_decisions_.end() && it->second == false) {
            control_active_[n] = true;
          }
        }
      }
    } else if (op == Opcode::kRegion) {
      // Region: active if any predecessor is active
      for (size_t i = 0; i < n->num_inputs(); ++i) {
        Node* pred = n->input(i);
        if (IsControlActive(pred)) {
          control_active_[n] = true;
          break;
        }
      }
    } else if (op == Opcode::kGoto) {
      // Goto: active if predecessor is active
      if (n->num_inputs() >= 1) {
        Node* pred = n->input(0);
        if (IsControlActive(pred)) {
          control_active_[n] = true;
        }
      }
    } else if (op == Opcode::kReturn) {
      // Return: active if predecessor is active
      if (n->num_inputs() >= 1) {
        Node* pred = n->input(0);
        if (IsControlActive(pred)) {
          control_active_[n] = true;
        }
      }
    } else if (op == Opcode::kHalt) {
      // Halt: active if predecessor is active
      if (n->num_inputs() >= 1) {
        Node* pred = n->input(0);
        if (IsControlActive(pred)) {
          control_active_[n] = true;
        }
      }
    }
  }
}

bool Interpreter::IsControlActive(const Node* ctrl) {
  if (!ctrl) return false;
  auto it = control_active_.find(ctrl);
  return it != control_active_.end() && it->second;
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
