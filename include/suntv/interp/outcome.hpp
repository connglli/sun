#pragma once
#include <optional>
#include <string>

#include "suntv/interp/heap.hpp"
#include "suntv/interp/value.hpp"

namespace sun {

struct Outcome {
  enum class Kind { Return, Throw } kind;
  std::optional<Value> return_value;
  std::string exception_kind;
  ConcreteHeap heap;

  std::string to_string() const;
};

}  // namespace sun
