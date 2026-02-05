#pragma once
#include <cstdint>
#include <string>

namespace sun {

using Ref = int32_t;

struct Value {
  enum class Kind { kI32, kI64, kBool, kRef, kNull };

  Kind kind;
  union {
    int32_t i32;
    int64_t i64;
    bool b;
    Ref ref;
  } data;

  // Factory methods
  static Value MakeI32(int32_t v);
  static Value MakeI64(int64_t v);
  static Value MakeBool(bool v);
  static Value MakeRef(Ref r);
  static Value MakeNull();

  // Accessors (with type checking)
  int32_t as_i32() const;
  int64_t as_i64() const;
  bool as_bool() const;
  Ref as_ref() const;

  // Type queries
  bool is_i32() const { return kind == Kind::kI32; }
  bool is_i64() const { return kind == Kind::kI64; }
  bool is_bool() const { return kind == Kind::kBool; }
  bool is_ref() const { return kind == Kind::kRef; }
  bool is_null() const { return kind == Kind::kNull; }

  // String representation
  std::string ToString() const;
};

}  // namespace sun
