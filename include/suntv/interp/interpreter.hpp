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
 * Executes graphs by traversing control edges dynamically, starting from Start
 * node. Value nodes are evaluated on-demand with memoization.
 *
 * Execution model:
 * 1. Start at Start node, follow control edges
 * 2. When control node needs a value (e.g., If condition), evaluate data
 * subgraph
 * 3. Value evaluation is recursive with memoization (DAG-aware)
 * 4. Loops are handled by traversing back-edges iteratively
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

  // Execution context: track which control predecessor was taken at each Region
  // This is needed for Phi node evaluation
  std::map<const Node*, const Node*> region_predecessor_;

  // Loop iteration tracking: Region -> iteration count (for loop termination)
  std::map<const Node*, int> loop_iterations_;

  // Maximum loop iterations before aborting (prevent infinite loops)
  static constexpr int kMaxLoopIterations = 10000;

  // Heap state
  ConcreteHeap heap_;

  // Main control flow traversal
  const Node* StepControl(const Node* ctrl);

  // Find control successor for a given control node
  const Node* FindControlSuccessor(const Node* ctrl) const;

  // Check if a Region is a loop header (has back-edge)
  bool IsLoopHeader(const Node* region) const;

  // Evaluate a value-producing node (with memoization)
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

  // Evaluate Conv2B (convert any value to boolean)
  Value EvalConv2B(const Node* n);

  // Evaluate no-op nodes (SafePoint, Opaque1, ParsePredicate)
  Value EvalNoOp(const Node* n);

  // Evaluate ThreadLocal (thread-local variable access)
  Value EvalThreadLocal(const Node* n);

  // Evaluate CallStaticJava (skip uncommon_trap)
  Value EvalCallStaticJava(const Node* n);

  // Evaluate Halt (abnormal termination)
  Value EvalHalt(const Node* n);

  // Memory operations
  Value EvalAllocate(const Node* n);
  Value EvalAllocateArray(const Node* n);
  Value EvalLoad(const Node* n);
  void EvalStore(const Node* n);
  void ProcessMemoryChain(const Node* mem);
};

}  // namespace sun
