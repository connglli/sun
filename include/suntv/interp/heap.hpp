#pragma once
#include <string>

#include "suntv/interp/value.hpp"

namespace sun {

class ConcreteHeap {
 public:
  Ref allocate_object();
  std::string dump() const;
};

}  // namespace sun
