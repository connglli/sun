#pragma once
#include <cstdint>

namespace sun {

using Ref = int32_t;

struct Value {
  enum class Kind { I32, I64, Bool, Ref, Null } kind;
  union {
    int32_t i32;
    int64_t i64;
    bool b;
    Ref ref;
  } data;

  static Value make_i32(int32_t v);
  static Value make_i64(int64_t v);
  static Value make_bool(bool v);
  static Value make_ref(Ref r);
  static Value make_null();
};

}  // namespace sun
