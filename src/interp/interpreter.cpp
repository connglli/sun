#include "suntv/interp/interpreter.hpp"

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
  std::vector<Node*> params = graph_.GetParameterNodes();
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
    throw std::runtime_error("Parm node should be handled separately");
  } else if (op == Opcode::kBool) {
    result = EvalBool(n);
  } else if (op == Opcode::kCMoveI || op == Opcode::kCMoveL ||
             op == Opcode::kCMoveP) {
    result = EvalCMove(n);
  } else if (op == Opcode::kPhi) {
    result = EvalPhi(n);
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
  if (!n->has_prop("index")) {
    throw std::runtime_error("Parm node missing 'index' property");
  }
  int32_t index = std::get<int32_t>(n->prop("index"));
  if (index < 0 || index >= static_cast<int32_t>(inputs.size())) {
    throw std::runtime_error("Parm index out of range");
  }
  return inputs[index];
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

  // HotSpot condition codes (simplified):
  // 1 = EQ, 2 = LT, 4 = GT, 8 = LE (LT|EQ), 5 = NE (LT|GT), etc.
  // Comparison returns: -1 (less), 0 (equal), 1 (greater)
  bool result = false;
  if (cmp_val < 0 && (mask & 2))
    result = true;  // LT
  else if (cmp_val == 0 && (mask & 1))
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
    }
  }
}

bool Interpreter::IsControlActive(const Node* ctrl) {
  if (!ctrl) return false;
  auto it = control_active_.find(ctrl);
  return it != control_active_.end() && it->second;
}

}  // namespace sun
