#include "suntv/interp/outcome.hpp"

#include <sstream>

namespace sun {

std::string Outcome::ToString() const {
  std::ostringstream oss;
  switch (kind) {
    case Kind::kReturn:
      oss << "Return(";
      if (return_value.has_value()) {
        oss << return_value->ToString();
      } else {
        oss << "void";
      }
      oss << ")";
      break;
    case Kind::kThrow:
      oss << "Throw(" << exception_kind << ")";
      break;
  }
  return oss.str();
}

}  // namespace sun
