#include "suntv/interp/evaluator.hpp"

#include <cstdlib>

namespace sun {

static Value WidenI32ToI64(Value v) {
  if (v.is_i32()) return Value::MakeI64(v.as_i32());
  return v;
}

// Arithmetic - Int32
Value Evaluator::EvalAddI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() + b.as_i32());
}

Value Evaluator::EvalSubI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() - b.as_i32());
}

Value Evaluator::EvalMulI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() * b.as_i32());
}

Value Evaluator::EvalDivI(Value a, Value b) {
  int32_t bv = b.as_i32();
  if (bv == 0) {
    throw EvalException("Division by zero");
  }
  return Value::MakeI32(a.as_i32() / bv);
}

Value Evaluator::EvalModI(Value a, Value b) {
  int32_t bv = b.as_i32();
  if (bv == 0) {
    throw EvalException("Modulo by zero");
  }
  return Value::MakeI32(a.as_i32() % bv);
}

Value Evaluator::EvalAbsI(Value a) {
  return Value::MakeI32(std::abs(a.as_i32()));
}

// Arithmetic - Int64
Value Evaluator::EvalAddL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() + b.as_i64());
}

Value Evaluator::EvalSubL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() - b.as_i64());
}

Value Evaluator::EvalMulL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() * b.as_i64());
}

Value Evaluator::EvalDivL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  int64_t bv = b.as_i64();
  if (bv == 0) {
    throw EvalException("Division by zero");
  }
  return Value::MakeI64(a.as_i64() / bv);
}

Value Evaluator::EvalModL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  int64_t bv = b.as_i64();
  if (bv == 0) {
    throw EvalException("Modulo by zero");
  }
  return Value::MakeI64(a.as_i64() % bv);
}

Value Evaluator::EvalAbsL(Value a) {
  a = WidenI32ToI64(a);
  return Value::MakeI64(std::llabs(a.as_i64()));
}

// Bitwise - Int32
Value Evaluator::EvalAndI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() & b.as_i32());
}

Value Evaluator::EvalOrI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() | b.as_i32());
}

Value Evaluator::EvalXorI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() ^ b.as_i32());
}

Value Evaluator::EvalLShiftI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() << (b.as_i32() & 0x1F));
}

Value Evaluator::EvalRShiftI(Value a, Value b) {
  return Value::MakeI32(a.as_i32() >> (b.as_i32() & 0x1F));
}

Value Evaluator::EvalURShiftI(Value a, Value b) {
  uint32_t ua = static_cast<uint32_t>(a.as_i32());
  return Value::MakeI32(static_cast<int32_t>(ua >> (b.as_i32() & 0x1F)));
}

// Bitwise - Int64
Value Evaluator::EvalAndL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() & b.as_i64());
}

Value Evaluator::EvalOrL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() | b.as_i64());
}

Value Evaluator::EvalXorL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() ^ b.as_i64());
}

Value Evaluator::EvalLShiftL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() << (b.as_i64() & 0x3F));
}

Value Evaluator::EvalRShiftL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeI64(a.as_i64() >> (b.as_i64() & 0x3F));
}

Value Evaluator::EvalURShiftL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  uint64_t ua = static_cast<uint64_t>(a.as_i64());
  return Value::MakeI64(static_cast<int64_t>(ua >> (b.as_i64() & 0x3F)));
}

// Comparison - Int32
Value Evaluator::EvalCmpEqI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() == b.as_i32());
}

Value Evaluator::EvalCmpNeI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() != b.as_i32());
}

Value Evaluator::EvalCmpLtI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() < b.as_i32());
}

Value Evaluator::EvalCmpLeI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() <= b.as_i32());
}

Value Evaluator::EvalCmpGtI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() > b.as_i32());
}

Value Evaluator::EvalCmpGeI(Value a, Value b) {
  return Value::MakeBool(a.as_i32() >= b.as_i32());
}

// Comparison - Int64
Value Evaluator::EvalCmpLtL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeBool(a.as_i64() < b.as_i64());
}

Value Evaluator::EvalCmpLeL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeBool(a.as_i64() <= b.as_i64());
}

Value Evaluator::EvalCmpGtL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeBool(a.as_i64() > b.as_i64());
}

Value Evaluator::EvalCmpGeL(Value a, Value b) {
  a = WidenI32ToI64(a);
  b = WidenI32ToI64(b);
  return Value::MakeBool(a.as_i64() >= b.as_i64());
}

// Comparison - Pointers
Value Evaluator::EvalCmpEqP(Value a, Value b) {
  return Value::MakeBool(a.as_ref() == b.as_ref());
}

Value Evaluator::EvalCmpNeP(Value a, Value b) {
  return Value::MakeBool(a.as_ref() != b.as_ref());
}

// Conversions
Value Evaluator::EvalConvI2L(Value a) {
  return Value::MakeI64(static_cast<int64_t>(a.as_i32()));
}

Value Evaluator::EvalConvL2I(Value a) {
  a = WidenI32ToI64(a);
  return Value::MakeI32(static_cast<int32_t>(a.as_i64()));
}

// Conditional move
Value Evaluator::EvalCMoveI(Value cond, Value true_val, Value false_val) {
  return cond.as_bool() ? true_val : false_val;
}

Value Evaluator::EvalCMoveL(Value cond, Value true_val, Value false_val) {
  return cond.as_bool() ? true_val : false_val;
}

Value Evaluator::EvalCMoveP(Value cond, Value true_val, Value false_val) {
  return cond.as_bool() ? true_val : false_val;
}

}  // namespace sun
