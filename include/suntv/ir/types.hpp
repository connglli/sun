#pragma once

#include <cstdint>
#include <string>

namespace sun {

/**
 * Type/stamp information for nodes (simplified for prototype).
 */
enum class TypeKind {
  kBottom,   // Bottom type (unreachable)
  kTop,      // Top type (unknown)
  kInt32,    // int32
  kInt64,    // int64
  kBool,     // boolean
  kPtr,      // pointer/reference
  kControl,  // control token
  kMemory,   // memory state
  kVoid      // void (no value)
};

class TypeStamp {
 public:
  TypeStamp() : kind_(TypeKind::kTop) {}
  explicit TypeStamp(TypeKind kind) : kind_(kind) {}

  TypeKind kind() const { return kind_; }

  bool IsInt32() const { return kind_ == TypeKind::kInt32; }
  bool IsInt64() const { return kind_ == TypeKind::kInt64; }
  bool IsBool() const { return kind_ == TypeKind::kBool; }
  bool IsPtr() const { return kind_ == TypeKind::kPtr; }
  bool IsControl() const { return kind_ == TypeKind::kControl; }
  bool IsMemory() const { return kind_ == TypeKind::kMemory; }

  std::string ToString() const;

 private:
  TypeKind kind_;
};

}  // namespace sun
