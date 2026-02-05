#include "suntv/interp/value.hpp"

namespace sun {

Value Value::make_i32(int32_t v) {
  Value val;
  val.kind = Kind::I32;
  val.data.i32 = v;
  return val;
}

Value Value::make_i64(int64_t v) {
  Value val;
  val.kind = Kind::I64;
  val.data.i64 = v;
  return val;
}

Value Value::make_bool(bool v) {
  Value val;
  val.kind = Kind::Bool;
  val.data.b = v;
  return val;
}

Value Value::make_ref(Ref r) {
  Value val;
  val.kind = Kind::Ref;
  val.data.ref = r;
  return val;
}

Value Value::make_null() {
  Value val;
  val.kind = Kind::Null;
  val.data.ref = 0;
  return val;
}

}  // namespace sun
