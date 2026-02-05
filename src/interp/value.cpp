#include "suntv/interp/value.hpp"

#include <sstream>
#include <stdexcept>

namespace sun {

Value Value::MakeI32(int32_t v) {
  Value val;
  val.kind = Kind::kI32;
  val.data.i32 = v;
  return val;
}

Value Value::MakeI64(int64_t v) {
  Value val;
  val.kind = Kind::kI64;
  val.data.i64 = v;
  return val;
}

Value Value::MakeBool(bool v) {
  Value val;
  val.kind = Kind::kBool;
  val.data.b = v;
  return val;
}

Value Value::MakeRef(Ref r) {
  Value val;
  val.kind = Kind::kRef;
  val.data.ref = r;
  return val;
}

Value Value::MakeNull() {
  Value val;
  val.kind = Kind::kNull;
  val.data.ref = 0;
  return val;
}

int32_t Value::as_i32() const {
  if (kind != Kind::kI32) {
    throw std::runtime_error("Value is not i32");
  }
  return data.i32;
}

int64_t Value::as_i64() const {
  if (kind != Kind::kI64) {
    throw std::runtime_error("Value is not i64");
  }
  return data.i64;
}

bool Value::as_bool() const {
  if (kind != Kind::kBool) {
    throw std::runtime_error("Value is not bool");
  }
  return data.b;
}

Ref Value::as_ref() const {
  if (kind != Kind::kRef && kind != Kind::kNull) {
    throw std::runtime_error("Value is not ref/null");
  }
  return data.ref;
}

std::string Value::ToString() const {
  std::ostringstream oss;
  switch (kind) {
    case Kind::kI32:
      oss << "i32:" << data.i32;
      break;
    case Kind::kI64:
      oss << "i64:" << data.i64;
      break;
    case Kind::kBool:
      oss << "bool:" << (data.b ? "true" : "false");
      break;
    case Kind::kRef:
      oss << "ref:" << data.ref;
      break;
    case Kind::kNull:
      oss << "null";
      break;
  }
  return oss.str();
}

}  // namespace sun
