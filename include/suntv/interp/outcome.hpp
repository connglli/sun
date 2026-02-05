#pragma once
#include <optional>
#include <string>

#include "suntv/interp/heap.hpp"
#include "suntv/interp/value.hpp"

namespace sun {

struct Outcome {
  enum class Kind { kReturn, kThrow };

  Kind kind;
  std::optional<Value> return_value;
  std::string exception_kind;
  ConcreteHeap heap;

  std::string ToString() const;
};

}  // namespace sun
