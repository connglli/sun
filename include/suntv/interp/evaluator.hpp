#pragma once

#include <stdexcept>

#include "suntv/interp/value.hpp"
#include "suntv/ir/node.hpp"

namespace sun {

/**
 * Evaluator for per-opcode concrete semantics.
 * Implements arithmetic, bitwise, comparison, and conversion operations.
 */
class Evaluator {
 public:
  // Arithmetic - Int32
  static Value EvalAddI(Value a, Value b);
  static Value EvalSubI(Value a, Value b);
  static Value EvalMulI(Value a, Value b);
  static Value EvalDivI(Value a, Value b);  // May throw on divide-by-zero
  static Value EvalModI(Value a, Value b);  // May throw on divide-by-zero
  static Value EvalAbsI(Value a);

  // Arithmetic - Int64
  static Value EvalAddL(Value a, Value b);
  static Value EvalSubL(Value a, Value b);
  static Value EvalMulL(Value a, Value b);
  static Value EvalDivL(Value a, Value b);
  static Value EvalModL(Value a, Value b);
  static Value EvalAbsL(Value a);

  // Bitwise - Int32
  static Value EvalAndI(Value a, Value b);
  static Value EvalOrI(Value a, Value b);
  static Value EvalXorI(Value a, Value b);
  static Value EvalLShiftI(Value a, Value b);
  static Value EvalRShiftI(Value a, Value b);   // Arithmetic right shift
  static Value EvalURShiftI(Value a, Value b);  // Logical right shift

  // Bitwise - Int64
  static Value EvalAndL(Value a, Value b);
  static Value EvalOrL(Value a, Value b);
  static Value EvalXorL(Value a, Value b);
  static Value EvalLShiftL(Value a, Value b);
  static Value EvalRShiftL(Value a, Value b);
  static Value EvalURShiftL(Value a, Value b);

  // Comparison (returns bool)
  static Value EvalCmpEqI(Value a, Value b);
  static Value EvalCmpNeI(Value a, Value b);
  static Value EvalCmpLtI(Value a, Value b);  // Signed less than
  static Value EvalCmpLeI(Value a, Value b);  // Signed less or equal
  static Value EvalCmpGtI(Value a, Value b);  // Signed greater than
  static Value EvalCmpGeI(Value a, Value b);  // Signed greater or equal
  static Value EvalCmpLtL(Value a, Value b);  // Signed less than
  static Value EvalCmpLeL(Value a, Value b);  // Signed less or equal
  static Value EvalCmpGtL(Value a, Value b);  // Signed greater than
  static Value EvalCmpGeL(Value a, Value b);  // Signed greater or equal
  static Value EvalCmpEqP(Value a, Value b);  // Pointer equality
  static Value EvalCmpNeP(Value a, Value b);  // Pointer inequality

  // Conversions
  static Value EvalConvI2L(Value a);  // Sign-extend i32 to i64
  static Value EvalConvL2I(Value a);  // Truncate i64 to i32

  // Conditional move
  static Value EvalCMoveI(Value cond, Value true_val, Value false_val);
  static Value EvalCMoveL(Value cond, Value true_val, Value false_val);
  static Value EvalCMoveP(Value cond, Value true_val, Value false_val);
};

/**
 * Exception thrown by evaluator for runtime errors.
 */
class EvalException : public std::runtime_error {
 public:
  explicit EvalException(const std::string& msg) : std::runtime_error(msg) {}
};

}  // namespace sun
