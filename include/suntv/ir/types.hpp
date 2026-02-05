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

  TypeKind GetKind() const { return kind_; }

  bool IsInt32() const { return kind_ == TypeKind::INT32; }
  bool IsInt64() const { return kind_ == TypeKind::INT64; }
  bool IsBool() const { return kind_ == TypeKind::BOOL; }
  bool IsPtr() const { return kind_ == TypeKind::PTR; }
  bool IsControl() const { return kind_ == TypeKind::CONTROL; }
  bool IsMemory() const { return kind_ == TypeKind::MEMORY; }

  std::string ToString() const;

 private:
  TypeKind kind_;
};

}  // namespace sun
