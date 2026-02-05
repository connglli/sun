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
  heap_ = ConcreteHeap();  // Reset heap

  // Cache parameter values first
  std::vector<Node*> params = graph_.GetParameterNodes();
  for (Node* parm_node : params) {
    Value parm_val = EvalParm(parm_node, inputs);
    value_cache_[parm_node] = parm_val;
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
  switch (op) {
    case Opcode::kCmpI:
      // For now, treat CmpI as equality check
      return Evaluator::EvalCmpEqI(a, b);
    case Opcode::kCmpL:
      return Evaluator::EvalCmpEqI(a, b);  // Simplified
    case Opcode::kCmpP:
      return Evaluator::EvalCmpEqP(a, b);
    default:
      throw std::runtime_error("Unsupported comparison opcode");
  }
}

Value Interpreter::EvalPhi(const Node* n) {
  // Simplified Phi evaluation: just return first input
  // Full implementation would track control flow and PC
  if (n->num_inputs() == 0) {
    throw std::runtime_error("Phi node has no inputs");
  }
  return EvalNode(n->input(0));
}

}  // namespace sun
