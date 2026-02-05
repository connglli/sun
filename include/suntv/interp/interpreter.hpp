#pragma once
#include <map>
#include <vector>

#include "suntv/interp/evaluator.hpp"
#include "suntv/interp/heap.hpp"
#include "suntv/interp/outcome.hpp"
#include "suntv/interp/value.hpp"

namespace sun {
class Graph;
class Node;

/**
 * Concrete interpreter for Sea-of-Nodes graphs.
 *
 * Executes graphs concretely by evaluating nodes in topological order,
 * tracking control flow and managing heap state.
 */
class Interpreter {
 public:
  explicit Interpreter(const Graph& g);

  /**
   * Execute the graph with given input values.
   * Returns the outcome (Return or Throw) with final heap state.
   */
  Outcome Execute(const std::vector<Value>& inputs);

 private:
  const Graph& graph_;

  // Memoization: node -> computed value
  std::map<const Node*, Value> value_cache_;

  // Control flow tracking: control node -> is active
  std::map<const Node*, bool> control_active_;

  // If node -> which branch was taken (true/false)
  std::map<const Node*, bool> if_decisions_;

  // Heap state
  ConcreteHeap heap_;

  // Evaluate a value-producing node
  Value EvalNode(const Node* n);

  // Evaluate constant node
  Value EvalConst(const Node* n);

  // Evaluate parameter node
  Value EvalParm(const Node* n, const std::vector<Value>& inputs);

  // Evaluate arithmetic/bitwise operation
  Value EvalArithOp(const Node* n);

  // Evaluate comparison operation
  Value EvalCmpOp(const Node* n);

  // Evaluate Bool node (comparison to boolean)
  Value EvalBool(const Node* n);

  // Evaluate CMoveI/L/P (conditional move)
  Value EvalCMove(const Node* n);

  // Evaluate Phi node (value merge)
  Value EvalPhi(const Node* n);

  // Memory operations
  Value EvalAllocate(const Node* n);
  Value EvalAllocateArray(const Node* n);
  Value EvalLoad(const Node* n);
  void EvalStore(const Node* n);
  void ProcessMemoryChain(const Node* mem);

  // Control flow helpers
  void ComputeControlFlow(const Node* start_node);
  bool IsControlActive(const Node* ctrl);
};

}  // namespace sun
