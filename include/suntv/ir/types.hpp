#pragma once

#include <cstdint>
#include <string>

namespace sun {

/**
 * Type/stamp information for nodes (simplified for prototype).
 */
enum class TypeKind {
  BOTTOM,   // Bottom type (unreachable)
  TOP,      // Top type (unknown)
  INT32,    // int32
  INT64,    // int64
  BOOL,     // boolean
  PTR,      // pointer/reference
  CONTROL,  // control token
  MEMORY,   // memory state
  VOID      // void (no value)
};

class TypeStamp {
 public:
  TypeStamp() : kind_(TypeKind::TOP) {}
  explicit TypeStamp(TypeKind kind) : kind_(kind) {}

  TypeKind kind() const { return kind_; }

  bool is_int32() const { return kind_ == TypeKind::INT32; }
  bool is_int64() const { return kind_ == TypeKind::INT64; }
  bool is_bool() const { return kind_ == TypeKind::BOOL; }
  bool is_ptr() const { return kind_ == TypeKind::PTR; }
  bool is_control() const { return kind_ == TypeKind::CONTROL; }
  bool is_memory() const { return kind_ == TypeKind::MEMORY; }

  std::string to_string() const;

 private:
  TypeKind kind_;
};

}  // namespace sun
