# Sea-of-Nodes (SoN) — A Comprehensive Guide

## Table of Contents

1. [Introduction](#introduction)
2. [What is Sea-of-Nodes?](#what-is-sea-of-nodes)
3. [Nodes: The Building Blocks](#nodes-the-building-blocks)
4. [Edges: Dependencies and Flow](#edges-dependencies-and-flow)
5. [How to Execute a Sea-of-Nodes Graph](#how-to-execute-a-sea-of-nodes-graph)
6. [Handling Different Node Types](#handling-different-node-types)
7. [Loops in Sea-of-Nodes](#loops-in-sea-of-nodes)
8. [Phi Nodes and Merging](#phi-nodes-and-merging)
9. [Memory Model](#memory-model)
10. [Comparison with Traditional IRs](#comparison-with-traditional-irs)
11. [References](#references)

---

## Introduction

The **Sea-of-Nodes (SoN)** is an intermediate representation (IR) used in optimizing compilers. It was pioneered by Clifford Click and Keith D. Cooper and is used in production systems including:

- **HotSpot C2** (Oracle/OpenJDK Java compiler)
- **V8 TurboFan** (Google Chrome JavaScript compiler)
- **GraalVM** (Oracle polyglot compiler)

This document provides a comprehensive introduction to Sea-of-Nodes, explaining what it is, how it works, and how to interpret/execute it.

---

## What is Sea-of-Nodes?

### The Core Idea

Traditional compilers use a **Control Flow Graph (CFG)** where:
- Instructions are grouped into **basic blocks**
- Blocks are connected by control flow edges
- Instructions within a block have a fixed sequential order

Sea-of-Nodes takes a different approach:
- Instructions become **nodes** in a directed acyclic graph (DAG)
- Nodes are connected by **edges** representing dependencies
- **No fixed execution order** except where dependencies require it
- Control flow, data flow, and memory dependencies are **unified** in one graph

### Key Benefits

1. **Optimization opportunities**: Floating nodes (pure computations) can be freely moved without consulting a separate CFG
2. **Unified representation**: Data dependencies, control dependencies, and memory dependencies are all explicit in the same graph
3. **Composable transformations**: Optimizations can be expressed as simple graph rewrite rules
4. **SSA form built-in**: Single Static Assignment is natural in this representation

### The "Sea" Metaphor

The name "Sea-of-Nodes" comes from the visualization: instead of rigid basic blocks in a CFG, you have a "sea" of computation nodes that float freely, constrained only by their dependency edges.

---

## Nodes: The Building Blocks

### What is a Node?

A **node** represents a single operation or control transfer in the program. Each node has:

1. **An opcode** — what operation it performs (e.g., `AddI`, `If`, `Load`, `Return`)
2. **Input edges** — dependencies on other nodes
3. **Output edges** — which nodes depend on this one (reverse edges, often called "uses")
4. **Properties** — additional metadata (e.g., constant values, field offsets)

### Node Categories

Nodes fall into several categories based on their semantic role:

#### 1. **Pure/Floating Nodes (Schema S0)**
Pure computations with no side effects:
- **Examples**: `AddI`, `SubL`, `MulI`, `CmpI`, `AndI`, `ConI` (constant)
- **Inputs**: Only value inputs (data dependencies)
- **Properties**:
  - No control or memory dependencies
  - Can be evaluated anywhere as long as inputs are available
  - Result depends only on input values
  - Can be freely moved during optimization

#### 2. **Control Nodes (Schema S1)**
Control transfer and branching:
- **Examples**: `Start`, `If`, `IfTrue`, `IfFalse`, `Goto`, `Return`
- **Inputs**: Control input (predecessor in control flow)
- **Properties**:
  - Define which code is reachable
  - Form the control flow backbone
  - Cannot be moved freely

#### 3. **Merge Nodes (Schema S2)**
Join points where control flow or values merge:
- **Examples**: `Region` (control merge), `Phi` (value merge)
- **Inputs**: Multiple control/value inputs from different predecessors
- **Properties**:
  - `Region`: Merges control flow paths
  - `Phi`: Selects between different values based on which control path was taken

#### 4. **Memory Nodes (Schema S3, S4, S5)**
Operations that read or write memory:
- **Examples**: `Load`, `Store`, `Allocate`, `AllocateArray`
- **Inputs**: Control, memory state, address/values
- **Properties**:
  - Ordered by memory dependencies
  - Guarded by control (only execute if control is reachable)
  - Form memory chains

#### 5. **Parameter Nodes**
Represent function inputs:
- **Examples**: `Parm`
- **Inputs**: `Start` node (control dependency)
- **Properties**: Represent external inputs to the graph

---

## Edges: Dependencies and Flow

### Edge Types

While edges in a Sea-of-Nodes graph are stored uniformly as pointers, they have **semantic types** based on what they represent:

#### 1. **Data Edges (Value Dependencies)**
Represent pure data flow:
```
AddI (inputs: [value1, value2])
      ↑         ↑
      |         |
    ConI(5)   Parm(0)
```
- Connect value producers to value consumers
- Form a DAG (no cycles)
- Can be evaluated in any topological order

#### 2. **Control Edges**
Represent control flow:
```
Start → If → IfTrue → Return
           ↘ IfFalse → Return
```
- Define execution order and reachability
- Form a graph with possible cycles (loops!)
- Must be traversed to determine what executes

#### 3. **Memory Edges**
Represent memory state dependencies:
```
Store1 → Store2 → Load
  ↑        ↑       ↑
  H0       H1      H2
```
- Form chains ordering memory operations
- Prevent incorrect reordering (e.g., load before store)
- Use Single Static Assignment for heap states

### Edge Positions Matter

The **position** of an edge encodes its meaning:

For most value nodes (e.g., `AddI`, `SubI`):
```
node.input[0] = first operand
node.input[1] = second operand
```

For control-dependent operations (e.g., `Store`):
```
node.input[0] = control edge (from predecessor control node)
node.input[1] = memory edge (from previous memory state)
node.input[2+] = value inputs (address, value to store, etc.)
```

For C2 graphs specifically (backward compatibility):
```
Many nodes have input[0] = nullptr (unused)
Actual inputs start at input[1]
```

---

## How to Execute a Sea-of-Nodes Graph

### The Execution Model

To execute (interpret) a Sea-of-Nodes graph:

1. **Start at the `Start` node** — this is the unique entry point
2. **Follow control edges** — traverse the control flow graph dynamically
3. **Evaluate value nodes on-demand** — when a control node needs a value, recursively evaluate the data subgraph
4. **Use memoization** — cache computed values to avoid redundant computation

### Execution Algorithm (Pseudocode)

```python
def execute_graph(graph):
    # Initialize state
    value_cache = {}  # Memoization for value nodes
    heap = initial_heap_state()

    # Start traversal
    current_control = graph.start_node

    # Traverse control flow
    while current_control.opcode != Return:
        current_control = step_control(current_control, value_cache, heap)

    # Extract result
    return_value = eval_value(current_control.value_input, value_cache)
    return (return_value, heap)

def step_control(ctrl, cache, heap):
    """Execute one control step, return next control node."""

    if ctrl.opcode == Start:
        # Start has one successor (usually a Region or If)
        return find_control_successor(ctrl)

    elif ctrl.opcode == Goto:
        # Unconditional jump
        return find_control_successor(ctrl)

    elif ctrl.opcode == If:
        # Evaluate condition (data subgraph)
        condition = eval_value(ctrl.condition_input, cache)
        # Follow true or false branch
        if condition:
            return ctrl.true_successor  # IfTrue node
        else:
            return ctrl.false_successor  # IfFalse node

    elif ctrl.opcode == IfTrue or ctrl.opcode == IfFalse:
        # These are just markers, continue to successor
        return find_control_successor(ctrl)

    elif ctrl.opcode == Region:
        # Control merge point - continue to successor
        # (Phi nodes attached to this Region are evaluated lazily when needed)
        return find_control_successor(ctrl)

    else:
        raise Exception(f"Unexpected control opcode: {ctrl.opcode}")

def eval_value(node, cache):
    """Evaluate a value node (with memoization)."""

    # Check cache first
    if node in cache:
        return cache[node]

    # Evaluate based on opcode
    if node.opcode == ConI:
        result = node.constant_value

    elif node.opcode == Parm:
        result = get_parameter(node.index)

    elif node.opcode == AddI:
        left = eval_value(node.input[0], cache)
        right = eval_value(node.input[1], cache)
        result = left + right

    elif node.opcode == Phi:
        # Phi selects based on which control predecessor was taken
        result = eval_phi(node, cache)

    # ... handle other opcodes ...

    # Cache and return
    cache[node] = result
    return result
```

### Key Principles

1. **Control flow is explicit** — follow control edges to determine what executes
2. **Data flow is implicit** — evaluate value nodes when needed
3. **Memoization is essential** — the graph is a DAG, nodes may have multiple uses
4. **No pre-scheduling required** — you don't need to build a CFG first

---

## Handling Different Node Types

### Pure Computation Nodes

**Examples**: `AddI`, `SubI`, `MulI`, `CmpI`, `ConI`

**Evaluation**:
```python
def eval_addi(node, cache):
    left = eval_value(node.input[0], cache)
    right = eval_value(node.input[1], cache)
    return left + right
```

**Key property**: No control or memory dependencies, can evaluate anywhere.

### Control Transfer Nodes

**Examples**: `If`, `IfTrue`, `IfFalse`, `Goto`, `Region`

**Evaluation**:
- These nodes don't compute values
- They determine which path to follow
- Execution traverses through them

**Special case: If nodes**
```python
def step_if(if_node, cache):
    # Evaluate the condition (may trigger recursive evaluation)
    cond = eval_value(if_node.condition_input, cache)

    # Choose successor based on condition
    if cond:
        return if_node.true_successor
    else:
        return if_node.false_successor
```

### Region Nodes (Control Merge)

A `Region` node merges multiple control flow paths:
```
     IfTrue ──┐
              ├──→ Region → ...
    IfFalse ──┘
```

**Execution**:
- When you reach a Region, you came from ONE of its predecessors
- Continue to the Region's successor
- Phi nodes use the Region to determine which value to select

### Phi Nodes (Value Merge)

A `Phi` node selects a value based on which control path was taken:
```
Phi node structure:
  input[0] = Region (control)
  input[1] = value from predecessor 1
  input[2] = value from predecessor 2
  ...
```

**Evaluation**:
```python
def eval_phi(phi, cache, current_control):
    region = phi.input[0]

    # Determine which control predecessor was taken
    predecessor_index = determine_active_predecessor(region, current_control)

    # Select corresponding value
    value_input = phi.input[predecessor_index]
    return eval_value(value_input, cache)
```

**Challenge**: You need to track which control path you came from!

### Memory Operations

**Load** (Schema S3):
```
Load node structure:
  input[0] = control edge (determines if load executes)
  input[1] = memory edge (heap state to read from)
  input[2+] = address/index to read
```

**Store** (Schema S4):
```
Store node structure:
  input[0] = control edge
  input[1] = memory edge (input heap state)
  input[2+] = address/value to write
  output: new memory state
```

**Execution**:
```python
def eval_load(load, cache, heap):
    address = eval_value(load.address_input, cache)
    return heap.read(address, load.field)

def eval_store(store, cache, heap):
    address = eval_value(store.address_input, cache)
    value = eval_value(store.value_input, cache)
    heap.write(address, store.field, value)
    return heap  # Returns updated heap state
```

---

## Loops in Sea-of-Nodes

Loops are one of the most interesting (and complex) parts of Sea-of-Nodes.

### Loop Structure

A loop in SoN has this general structure:
```
     entry
       ↓
    Region (loop header)
       ↓
      If (loop test)
     /  \
  IfTrue IfFalse
    |      |
   body   exit
    |
   (back edge) ─→ Region
```

**Key characteristic**: The Region has a **back-edge** from within the loop body.

### Loop Phi Nodes

Loop variables use Phi nodes:
```
Phi node:
  input[0] = Region (loop header)
  input[1] = initial value (from outside loop)
  input[2] = updated value (from loop body, via back-edge)
```

**Example** (for i = 0; i < 10; i++):
```
Phi_i:
  input[0] = LoopRegion
  input[1] = ConI(0)           // initial: i = 0
  input[2] = AddI(Phi_i, 1)    // update: i++
```

**Note the cycle**: `Phi_i` depends on `AddI`, which depends on `Phi_i`!

### How to Execute Loops

**Option 1: Iterative Evaluation**
```python
def execute_loop(region):
    # Initialize loop variables with entry values
    phi_values = {}
    for phi in region.phi_nodes:
        phi_values[phi] = eval_value(phi.entry_input, cache)

    # Iterate until exit
    while True:
        # Update cache with current phi values
        for phi, value in phi_values.items():
            cache[phi] = value

        # Execute loop body
        exit_taken = execute_loop_body(region)

        if exit_taken:
            break

        # Update phi values from back-edge
        for phi in region.phi_nodes:
            phi_values[phi] = eval_value(phi.backedge_input, cache)
```

**Option 2: Natural Traversal**
Simply follow the back-edge! The control flow naturally loops:
```python
def step_control(ctrl, cache, heap):
    # ... normal cases ...

    elif ctrl.opcode == Region:
        # Check if we've been here before (loop detection)
        if is_loop_header(ctrl):
            # Update phi values and continue
            update_loop_phis(ctrl, cache)

        return find_control_successor(ctrl)
```

The key insight: **by traversing the back-edge, you naturally iterate the loop**!

---

## Phi Nodes and Merging

### The Phi Selection Problem

When you reach a Phi node, you need to know **which control predecessor was active**.

**Example**:
```
If (x > 0)
  a = 1
else
  a = 2
return a
```

Becomes:
```
      If(x>0)
      /     \
  IfTrue  IfFalse
   [a=1]   [a=2]
      \     /
      Region
        |
      Phi(1, 2) → use a
```

When evaluating the Phi, you need to know: did we come from IfTrue (select 1) or IfFalse (select 2)?

### Solution 1: Track Control Path

Maintain an **execution context** that records which control path was taken:
```python
class ExecutionContext:
    def __init__(self):
        self.current_control = None  # Current control node
        self.control_path = []       # History of control nodes visited

    def which_predecessor(self, region):
        # Look backward in control_path to find which region input we came from
        for ctrl in reversed(self.control_path):
            for i, pred in enumerate(region.inputs):
                if ctrl == pred or is_successor_of(ctrl, pred):
                    return i
        raise Exception("Cannot determine predecessor")
```

### Solution 2: Immediate Predecessor Tracking

For simple cases (non-loop), you can track just the immediate predecessor:
```python
def step_control(ctrl, cache, heap):
    if ctrl.opcode == Region:
        # Record which input we came from
        # (by tracking the predecessor control node)
        region_predecessor[ctrl] = previous_control

    # ... continue execution ...

def eval_phi(phi, cache):
    region = phi.input[0]
    pred = region_predecessor[region]

    # Find which input corresponds to this predecessor
    for i, reg_input in enumerate(region.inputs):
        if reg_input == pred:
            return eval_value(phi.input[i], cache)
```

---

## Memory Model

### Heap State as SSA

Memory is modeled using **Single Static Assignment** for heap states:

```
H0 → Store(obj, field, val) → H1 → Load(obj, field) → H2
```

Each memory operation produces a **new heap state**.

### Memory Chains

Stores and loads are connected by memory edges:
```
Store1 (produces H1)
   ↓
Store2 (consumes H1, produces H2)
   ↓
Load (consumes H2)
```

This ordering ensures:
- Loads see previous stores
- Stores don't get reordered incorrectly

### Aliasing

Sun uses **field-based aliasing**:
- Different fields can have independent memory chains
- Same field across different objects share a chain (conservative)
- Null analysis can disambiguate some cases

---

## Comparison with Traditional IRs

### Control Flow Graph (CFG)

**CFG structure**:
```
BasicBlock1:
  x = load(addr)
  y = x + 5
  if (y > 0) goto BB2 else goto BB3

BasicBlock2:
  z = y * 2
  goto BB4

BasicBlock3:
  z = y + 1
  goto BB4

BasicBlock4:
  return z
```

**Characteristics**:
- Instructions ordered within blocks
- Control flow between blocks
- Data flow implicit (variables/registers)

**SoN structure** (same program):
```
Start → If(Load(addr)+5 > 0) → IfTrue → Region → Return(Phi)
                              ↘ IfFalse ↗

Where Phi selects: MulI(y,2) vs AddI(y,1)
```

**Characteristics**:
- No basic blocks
- Instructions (nodes) unordered except by dependencies
- Control flow, data flow, memory flow all explicit as edges

### SSA Form

**Traditional SSA** (in CFG):
```
x1 = ...
if (...) {
  x2 = ... x1 ...
} else {
  x3 = ... x1 ...
}
x4 = φ(x2, x3)
```

**SoN**: Phi nodes are natural, part of the graph structure.

---

## References

### Foundational Papers

1. **Cliff Click (1995)** — "Combining Analyses, Combining Optimizations"
   - Original Sea-of-Nodes paper
   - Describes the IR and optimization framework

2. **Cliff Click & Michael Paleczny (1995)** — "A Simple Graph-Based Intermediate Representation"
   - Detailed description of the IR
   - Global code motion algorithm

### Implementations

1. **HotSpot C2** — Java HotSpot compiler
   - Production JIT compiler using SoN
   - `opto/` directory in OpenJDK source

2. **V8 TurboFan** — JavaScript compiler
   - Sea-of-Nodes based optimizing compiler
   - https://v8.dev/docs/turbofan

3. **GraalVM** — Polyglot compiler
   - Java-based compiler using SoN
   - https://github.com/oracle/graal

### Educational Resources

1. **"Sea-of-Nodes" blog post by Fedor Indutny**
   - https://darksi.de/d.sea-of-nodes/
   - Excellent introduction with visualizations

2. **"A Simple Showcase for the Sea-of-Nodes Compiler IR"**
   - https://github.com/SeaOfNodes/Simple
   - Step-by-step implementation tutorial

3. **This Project (Sun)**
   - Translation validation for C2 graphs
   - See `DOCS.md` for formal semantics
   - See `NODES.md` for node catalog

---

## Summary

**Sea-of-Nodes** is a powerful IR that:
- Unifies control flow, data flow, and memory dependencies in one graph
- Enables aggressive optimization through graph rewriting
- Naturally represents SSA form
- Used in production compilers (C2, V8, Graal)

**To execute a SoN graph**:
1. Start at `Start` node
2. Follow control edges dynamically
3. Evaluate value nodes on-demand (with memoization)
4. Handle loops by traversing back-edges iteratively
5. Track control context for Phi node selection

**Key insight**: Unlike CFGs where you schedule instructions first, SoN lets you **traverse control flow and evaluate data dependencies on-the-fly**.
