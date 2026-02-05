#include "suntv/interp/interpreter.hpp"

#include "suntv/ir/graph.hpp"

namespace sun {

Interpreter::Interpreter(const Graph& g) : graph_(g) {}

Outcome Interpreter::execute(const std::vector<Value>& inputs) {
  (void)inputs;
  Outcome o;
  o.kind = Outcome::Kind::Return;
  o.return_value = Value::make_i32(0);
  return o;  // Stub
}

}  // namespace sun
