#include "suntv/ir/types.hpp"

namespace sun {

std::string TypeStamp::to_string() const {
  switch (kind_) {
    case TypeKind::BOTTOM:
      return "⊥";
    case TypeKind::TOP:
      return "⊤";
    case TypeKind::INT32:
      return "int32";
    case TypeKind::INT64:
      return "int64";
    case TypeKind::BOOL:
      return "bool";
    case TypeKind::PTR:
      return "ptr";
    case TypeKind::CONTROL:
      return "ctrl";
    case TypeKind::MEMORY:
      return "mem";
    case TypeKind::VOID:
      return "void";
    default:
      return "unknown";
  }
}

}  // namespace sun
