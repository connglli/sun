#include "suntv/ir/types.hpp"

namespace sun {

std::string TypeStamp::ToString() const {
  switch (kind_) {
    case TypeKind::kBottom:
      return "⊥";
    case TypeKind::kTop:
      return "⊤";
    case TypeKind::kInt32:
      return "int32";
    case TypeKind::kInt64:
      return "int64";
    case TypeKind::kBool:
      return "bool";
    case TypeKind::kPtr:
      return "ptr";
    case TypeKind::kControl:
      return "ctrl";
    case TypeKind::kMemory:
      return "mem";
    case TypeKind::kVoid:
      return "void";
    default:
      return "unknown";
  }
}

}  // namespace sun
