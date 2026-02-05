#pragma once
#include <vector>

#include "suntv/interp/outcome.hpp"

namespace sun {
class Graph;

class Interpreter {
 public:
  explicit Interpreter(const Graph& g);
  Outcome execute(const std::vector<Value>& inputs);

 private:
  const Graph& graph_;
};

}  // namespace sun
