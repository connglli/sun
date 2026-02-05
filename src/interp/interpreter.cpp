#include "suntv/interp/interpreter.hpp"

#include "suntv/ir/graph.hpp"

namespace sun {

Interpreter::Interpreter(const Graph& g) : graph_(g) {}

Outcome Interpreter::execute(const std::vector<Value>& inputs) {
  (void)inputs;
  Outcome o;
  o.kind = Outcome::Kind::kReturn;
  o.return_value = Value::MakeI32(0);
  return o;  // Stub
}

}  // namespace sun
