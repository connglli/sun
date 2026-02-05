# NODES.md — C2 Sea-of-Nodes (SoN) Node Catalog

This document catalogs SoN/Ideal-Graph **node kinds** and, for each node, specifies:
- its **input interface** (Control/Memory/Value dependencies),
- its **outputs** (Value/Memory/Control/Outcome), and
- a short **operational meaning** in the `PC` + functional-heap style used by `suni`/`suntv`.

The full opcode universe is **JDK-version dependent**. This file starts with a **curated superset of common C2 nodes**; extend it by regenerating from your target JDK’s `src/hotspot/share/opto/classes.hpp`.

Prototype fragment assumptions for `suni`/`suntv` (nodes outside are documented but out-of-scope):
- loop-free, call-free, deopt-free, volatile-free, synchronization-free, fp-free; allocation allowed.

## Semantics schemas

### S0: Pure
- **Inputs**: Value inputs: v1..vk
- **Outputs**: Value output: v
- **Meaning**: Pure computation: v := f(v1..vk). No Control/Memory inputs.

### S1: Control
- **Inputs**: Control input: c (+ optional value cond)
- **Outputs**: Control output: c'
- **Meaning**: Control structure/transfer; defines reachability predicate PC on successors.

### S2: Phi/Region
- **Inputs**: Control preds: c1..ck; incoming values/states: x1..xk
- **Outputs**: Merged value/state: x
- **Meaning**: Merged selection: x := ite(PC(c1), x1, ite(PC(c2), x2, ...)). Structured merges assume disjoint PCs.

### S3: Load
- **Inputs**: Control: c; Memory in: H; Address/base: a; (props: kind/field/index)
- **Outputs**: Value output: v
- **Meaning**: Guarded load: v := ite(PC(c), read(H,a,prop), default_val).

### S4: Store
- **Inputs**: Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index)
- **Outputs**: Memory output: H'
- **Meaning**: Guarded store: H' := ite(PC(c), write(H,a,prop,v), H).

### S5: Allocate
- **Inputs**: Control: c; Memory in: H; (props: klass/len)
- **Outputs**: Value output: r (Ref), Memory out: H', next_id'
- **Meaning**: Guarded allocation: if PC(c) then r := next_id; next_id := next_id+1; initialize object/array in H'.

### S6: Return
- **Inputs**: Control: c; Memory in: H; (optional) value v
- **Outputs**: Outcome
- **Meaning**: Program outcome. For multiple returns, merge by PC into ite-selected value/heap.

### S7: Call/Runtime
- **Inputs**: Control: c; Memory in: H; args...
- **Outputs**: Value(s)+Memory+Control
- **Meaning**: Call-like node (out of prototype scope: call-free).

### S8: Loop
- **Inputs**: Control+Phi structures
- **Outputs**: Control+Phi
- **Meaning**: Loop-related control (out of prototype scope: loop-free).

### S9: Barrier/Volatile/Sync
- **Inputs**: Control+Memory
- **Outputs**: Memory/Control
- **Meaning**: Memory barrier / volatile / sync (out of prototype scope).

### S10: FP
- **Inputs**: Value inputs
- **Outputs**: Value output
- **Meaning**: Floating-point node (out of prototype scope: fp-free).

### S11: Vector
- **Inputs**: Vector inputs (and sometimes Memory/Mask)
- **Outputs**: Vector output (and sometimes Memory)
- **Meaning**: Vector/SIMD node (out of prototype scope for prototype fragment).

## Node list

| Node | Schema | Category | Inputs | Outputs | Short description |
|---|---|---|---|---|---|
| `AbsD` | S10 | FP | Value inputs | Value output | absolute value of double (fp-free prototype: out of scope) |
| `AbsF` | S10 | FP | Value inputs | Value output | absolute value of float (fp-free prototype: out of scope) |
| `AbsI` | S0 | Pure | Value inputs: v1..vk | Value output: v | absolute value of 32-bit integer |
| `AbsL` | S0 | Pure | Value inputs: v1..vk | Value output: v | absolute value of 64-bit integer |
| `AddD` | S10 | FP | Value inputs | Value output | add two doubles (fp-free prototype: out of scope) |
| `AddF` | S10 | FP | Value inputs | Value output | add two floats (fp-free prototype: out of scope) |
| `AddI` | S0 | Pure | Value inputs: v1..vk | Value output: v | add two 32-bit integers (wraparound) |
| `AddL` | S0 | Pure | Value inputs: v1..vk | Value output: v | add two 64-bit integers (wraparound) |
| `AddP` | S0 | Pure | Value inputs: v1..vk | Value output: v | compute pointer/address addition (object/address arithmetic) |
| `Allocate` | S5 | Allocate | Control: c; Memory in: H; (props: klass/len) | Value output: r (Ref), Memory out: H', next_id' | allocate a new object (fresh reference) |
| `AllocateArray` | S5 | Allocate | Control: c; Memory in: H; (props: klass/len) | Value output: r (Ref), Memory out: H', next_id' | allocate a new array (fresh reference) with given length |
| `AndI` | S0 | Pure | Value inputs: v1..vk | Value output: v | bitwise AND of two 32-bit integers |
| `AndL` | S0 | Pure | Value inputs: v1..vk | Value output: v | bitwise AND of two 64-bit integers |
| `AryEq` | S0 | Pure | Value inputs: v1..vk | Value output: v | compare two arrays for element-wise equality (often intrinsic; may be out of scope if it implies loops/calls) |
| `Bool` | S0 | Pure | Value inputs: v1..vk | Value output: v | convert/interpret a compare result as a boolean predicate |
| `BoxLock` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | represents a stack lock box for synchronization (sync-free prototype: out of scope) |
| `CProj` | S0 | Pure | Value inputs: v1..vk | Value output: v | control projection from a multi-control node (e.g., Call/Catch); selects a control successor |
| `CallJava` | S7 | Call/Runtime | Control: c; Memory in: H; args... | Value(s)+Memory+Control | call a Java method (call-free prototype: out of scope) |
| `CallLeaf` | S7 | Call/Runtime | Control: c; Memory in: H; args... | Value(s)+Memory+Control | call a VM leaf routine (call-free prototype: out of scope) |
| `CallRuntime` | S7 | Call/Runtime | Control: c; Memory in: H; args... | Value(s)+Memory+Control | call into the VM runtime (call-free prototype: out of scope) |
| `CallStaticJava` | S7 | Call/Runtime | Control: c; Memory in: H; args... | Value(s)+Memory+Control | call a resolved static Java method (call-free prototype: out of scope) |
| `CastII` | S0 | Pure | Value inputs: v1..vk | Value output: v | type/range cast on int (may add constraints; value-preserving in prototype) |
| `CastLL` | S0 | Pure | Value inputs: v1..vk | Value output: v | type/range cast on long (may add constraints; value-preserving in prototype) |
| `CastPP` | S0 | Pure | Value inputs: v1..vk | Value output: v | type/nullness cast on reference (constraint node; value-preserving in prototype) |
| `CastX2P` | S0 | Pure | Value inputs: v1..vk | Value output: v | cast machine integer to pointer/reference representation (platform-specific) |
| `CastP2X` | S0 | Pure | Value inputs: v1..vk | Value output: v | cast pointer/reference representation to machine integer (platform-specific) |
| `Catch` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | exception handler region for a call (call-free prototype: out of scope) |
| `CatchProj` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | projection from Catch for a specific exception path (call-free prototype: out of scope) |
| `CheckCastPP` | S0 | Pure | Value inputs: v1..vk | Value output: v | runtime reference type check/cast (may throw; typically out of scope in call-free core unless modeled) |
| `ClearArray` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | clear/zero an array range (memory effect; may imply loops/calls; usually out of scope initially) |
| `CMoveI` | S0 | Pure | Value inputs: v1..vk | Value output: v | conditional move/select between two int values |
| `CMoveL` | S0 | Pure | Value inputs: v1..vk | Value output: v | conditional move/select between two long values |
| `CMoveP` | S0 | Pure | Value inputs: v1..vk | Value output: v | conditional move/select between two reference values |
| `CmpI` | S0 | Pure | Value inputs: v1..vk | Value output: v | compare two 32-bit integers (produces compare code used by Bool) |
| `CmpL` | S0 | Pure | Value inputs: v1..vk | Value output: v | compare two 64-bit integers (produces compare code used by Bool) |
| `CmpP` | S0 | Pure | Value inputs: v1..vk | Value output: v | compare two references/addresses |
| `CmpU` | S0 | Pure | Value inputs: v1..vk | Value output: v | unsigned compare of two 32-bit integers |
| `CmpUL` | S0 | Pure | Value inputs: v1..vk | Value output: v | unsigned compare of two 64-bit integers |
| `CmpF` | S10 | FP | Value inputs | Value output | compare two floats (fp-free prototype: out of scope) |
| `CmpD` | S10 | FP | Value inputs | Value output | compare two doubles (fp-free prototype: out of scope) |
| `CmpN` | S0 | Pure | Value inputs: v1..vk | Value output: v | compare compressed null / narrow oop values (platform-specific) |
| `ConI` | S0 | Pure | Value inputs: v1..vk | Value output: v | 32-bit integer constant |
| `ConL` | S0 | Pure | Value inputs: v1..vk | Value output: v | 64-bit integer constant |
| `ConP` | S0 | Pure | Value inputs: v1..vk | Value output: v | reference constant (e.g., null or metadata pointer) |
| `ConF` | S10 | FP | Value inputs | Value output | float constant (fp-free prototype: out of scope) |
| `ConD` | S10 | FP | Value inputs | Value output | double constant (fp-free prototype: out of scope) |
| `ConvI2L` | S0 | Pure | Value inputs: v1..vk | Value output: v | sign-extend 32-bit int to 64-bit long |
| `ConvL2I` | S0 | Pure | Value inputs: v1..vk | Value output: v | truncate 64-bit long to 32-bit int |
| `ConvI2F` | S10 | FP | Value inputs | Value output | convert int to float (fp-free prototype: out of scope) |
| `ConvF2I` | S10 | FP | Value inputs | Value output | convert float to int (fp-free prototype: out of scope) |
| `ConvL2D` | S10 | FP | Value inputs | Value output | convert long to double (fp-free prototype: out of scope) |
| `ConvD2L` | S10 | FP | Value inputs | Value output | convert double to long (fp-free prototype: out of scope) |
| `DivI` | S0 | Pure | Value inputs: v1..vk | Value output: v | signed division of two 32-bit integers (may throw on /0) |
| `DivL` | S0 | Pure | Value inputs: v1..vk | Value output: v | signed division of two 64-bit integers (may throw on /0) |
| `DivF` | S10 | FP | Value inputs | Value output | float division (fp-free prototype: out of scope) |
| `DivD` | S10 | FP | Value inputs | Value output | double division (fp-free prototype: out of scope) |
| `AndV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | bitwise AND of vectors (vector prototype: out of scope) |
| `OrV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | bitwise OR of vectors (vector prototype: out of scope) |
| `XorV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | bitwise XOR of vectors (vector prototype: out of scope) |
| `AddV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | vector addition (vector prototype: out of scope) |
| `SubV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | vector subtraction (vector prototype: out of scope) |
| `MulV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | vector multiplication (vector prototype: out of scope) |
| `DivV` | S11 | Vector | Vector inputs (and sometimes Memory/Mask) | Vector output (and sometimes Memory) | vector division (vector prototype: out of scope) |
| `If` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | conditional branch based on boolean predicate |
| `IfTrue` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | true-successor control projection of an If |
| `IfFalse` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | false-successor control projection of an If |
| `Goto` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | unconditional branch / control edge |
| `Region` | S2 | Phi/Region | Control preds: c1..ck; incoming values/states: x1..xk | Merged value/state: x | control-flow merge point (join) |
| `Phi` | S2 | Phi/Region | Control preds: c1..ck; incoming values/states: x1..xk | Merged value/state: x | select value from predecessors at a Region (SSA phi) |
| `MergeMem` | S2 | Phi/Region | Control preds: c1..ck; incoming values/states: x1..xk | Merged value/state: x | merge memory states from predecessors at a Region |
| `Return` | S6 | Return | Control: c; Memory in: H; (optional) value v | Outcome | function/method return (produces program outcome) |
| `Start` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | method entry control node |
| `Root` | S1 | Control | Control input: c (+ optional value cond) | Control output: c' | graph root/anchor node (scheduling/graph management; not an executable op) |
| `LoadB` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load signed byte from memory |
| `LoadUB` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load unsigned byte from memory |
| `LoadS` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load signed 16-bit short from memory |
| `LoadUS` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load unsigned 16-bit short/char from memory |
| `LoadI` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load 32-bit int from memory |
| `LoadL` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load 64-bit long from memory |
| `LoadP` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load reference/pointer from memory |
| `LoadN` | S3 | Load | Control: c; Memory in: H; Address/base: a; (props: kind/field/index) | Value output: v | load narrow (compressed) reference from memory |
| `StoreB` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store byte to memory |
| `StoreC` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store 16-bit char to memory |
| `StoreI` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store 32-bit int to memory |
| `StoreL` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store 64-bit long to memory |
| `StoreP` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store reference/pointer to memory |
| `StoreN` | S4 | Store | Control: c; Memory in: H; Address/base: a; Value: v; (props: kind/field/index) | Memory output: H' | store narrow (compressed) reference to memory |
| `MemBarAcquire` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | acquire memory barrier (volatile/sync semantics; out of scope) |
| `MemBarRelease` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | release memory barrier (volatile/sync semantics; out of scope) |
| `MemBarVolatile` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | full volatile memory barrier (out of scope) |
| `MemBarStoreStore` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | store-store barrier (out of scope) |
| `MemBarCPUOrder` | S9 | Barrier/Volatile/Sync | Control+Memory | Memory/Control | cpu-order barrier (out of scope) |
| `Loop` | S8 | Loop | Control+Phi structures | Control+Phi | generic loop header/control (loop-free prototype: out of scope) |
| `CountedLoop` | S8 | Loop | Control+Phi structures | Control+Phi | counted loop header/control (loop-free prototype: out of scope) |
| `CountedLoopEnd` | S8 | Loop | Control+Phi structures | Control+Phi | counted loop backedge/end (loop-free prototype: out of scope) |

## Regeneration recipe (agent note)

To make this file exhaustive for a specific JDK:
1. Extract opcode names from `src/hotspot/share/opto/classes.hpp` (the `macro(x)` table).
2. Replace the `nodes_raw` list used to generate this document.
3. Re-run the generator (or update the table manually).
