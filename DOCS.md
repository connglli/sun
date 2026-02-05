# The Sun Project — Translation Validation for Sea of Nodes


## 1. Background: LLVM and Alive / Alive2

- **Alive / Alive2** is a *translation validation* framework for LLVM IR optimizations.
- Rather than proving the whole compiler correct, it proves (or refutes) that a **specific transformation instance** preserves semantics.
- Alive2 encodes LLVM IR semantics into SMT constraints and checks equivalence/refinement, producing counterexamples when incorrect.

This is the conceptual template for validating optimizations in C2’s SoN graphs.


## 2. HotSpot C2 Sea of Nodes: Recap

C2 uses a graph-based IR called **Sea of Nodes (SoN)**, coined by Cliff Click. Key features:

- A single graph represents **data flow**, **control flow**, and **memory flow**.
- **Floating** nodes (pure computations) are movable subject to dependencies.
- **Fixed** nodes (side effects / control transfers) are constrained by control and memory dependencies.
- Memory is explicit via **memory edges** and merge nodes (e.g., `MergeMem`), enabling global optimization prior to final scheduling.

SoN also inspired other compilers like Graal and V8 TurboFan.

A simple SoN implementation can be found in the [Simple](https://github.com/SeaOfNodes/Simple) repository.


## 3. Translation Validation on SoN: Assumptions

Initial prototype restricts graphs to:

✅ fp-free<br/>
✅ loop-free<br/>
✅ call-free<br/>
✅ deopt-free<br/>
✅ volatile-free<br/>
✅ synchronization-free<br/>

✅ exception allowed
✅ allocation allowed

This removes concurrency/JMM and runtime calls while keeping meaningful memory + allocation behavior.


## 4. Translation Validation on SoN: Correctness Statement

Given two SoN graphs:

- **G1**: before optimization
- **G2**: after optimization

Translation validation compares the graphs by their **observable behavior** (or, equivalently, by a behavioral equivalence/refinement relation), not by syntactic equality of intermediate constraints.

### 4.1 Outcomes
An execution yields an **Outcome**:
- `Return(v, H)`  — returns value `v` and heap `H`
- `Throw(k, H)` — throws exception kind `k` and heap `H`
- (later) `Deopt(s)` — uncommon trap/deopt with state `s`

### 4.2 Heap Equivalence
For heap-manipulating programs with allocation, the comparison uses **heap equivalence modulo allocation renaming** (bijection/permutation on freshly allocated references). This supports differences in allocation identity and unreachable garbage while preserving observable behavior.

### 4.3 Validation Query Form
TV is expressed by encoding **non-equivalence** and querying the SMT solver:
- `SAT` ⇒ counterexample input
- `UNSAT` ⇒ validated for the modeled fragment

## 5. Translation Validation on SoN: Formal Model

This section specifies the formal model of the translation validation, based on a **big-step operational semantics**.

### 5.1 Basic Domains
Let:

- `Int32`, `Int64`, `Bool` be standard bitvector domains (SMT BV).
- `Ref` be an uninterpreted or bounded reference domain; prototype uses **bounded fresh refs** via an allocation counter.
- `Val` be a tagged union (or multi-sorted encoding) for:
  - `i32`, `i64`, `bool`, `ref`
  - (future extension) `f32`, `f64`
- `Klass` / `TypeTag` optional; used only if needed for allocation typing; can be uninterpreted in the prototype.

**Locations**. We model heap locations as:

- Object field location: `LocF = (Ref, FieldId)`
- Array element location: `LocA = (Ref, Index)` (Index typically `Int32`)
- Optionally: array length `Len(Ref)` as a separate map.

For the initial set, it is enough to support:
- field stores/loads with a `FieldId` property, and/or
- array element stores/loads with an index term.

### 5.2 Heap Model
A heap is a functional map (SMT array-like):

- `Hf : Ref × FieldId → Val`  (fields)
- `Ha : Ref × Int32 → Val`    (array elements)  (optional if arrays included)
- `Hlen : Ref → Int32`        (array lengths)   (optional if arrays included)

We write:
- `read_f(H, r, fid)` and `write_f(H, r, fid, v)`
- similarly for arrays if present.

### 5.3 Symbolic State
A symbolic state is:

- `next : Int32`   (fresh allocation counter)
- `H : Heap`       (heap maps)
- `PC : CtrlNode → Bool` (path condition / reachability predicate per control node)

We treat `PC` as a derived function computed from control structure, but semantically it functions as a guard on side effects.


## 6. Translation Validation on SoN: Canonical Graph Form

A SoN graph is a finite set of nodes `N` with directed edges.

Each node has:
- `op(n)` — opcode
- `in(n, k)` — kth input edge (value/control/memory depending on opcode)
- `prop(n)` — properties (constants, field id, array element kind, etc.)
- `type(n)` — (optional) stamp/type (used only for constraints, not required initially)

We assume:
- **Acyclicity** (loop-free): the dependency graph is acyclic modulo `Region/Phi` merge structure.
- **Single-entry**: one `Start` node.
- **Well-formed merges**: `Phi` nodes are attached to a corresponding `Region` and select among predecessor values.


## 7. Translation Validation on SoN: Big-Step Operational Semantics

We define a symbolic evaluation function:

- `⟦n⟧_v` yields an SMT term for a **value node** `n`
- `⟦m⟧_H` yields an SMT term for a **heap state** produced by a **memory node** `m`
- `PC(c)` yields a boolean SMT term for a **control node** `c`

In implementation, these are memoized topological computations.

### 7.1 Control Reachability (`PC`) Rules
Let `Start` be the unique entry control node.

#### (PC-Start)
`PC(Start) = true`

#### (PC-Goto)
If `Goto` has predecessor control `c`:
`PC(Goto) = PC(c)`

#### (PC-IfTrue/IfFalse)
If `If` has predecessor control `c` and condition value term `b = ⟦cond⟧_v`:

- `PC(IfTrue)  = PC(c) ∧ b`
- `PC(IfFalse) = PC(c) ∧ ¬b`

#### (PC-Region)
If `Region` has control predecessors `c1..ck`:
`PC(Region) = PC(c1) ∨ ... ∨ PC(ck)`

> **Well-formedness (structured merges)**: In the intended fragment, Region merges correspond to structured `If` splits, so predecessor PCs should be pairwise disjoint:
>
> `WF_Region ≜ ∧_{i≠j} ¬(PC(ci) ∧ PC(cj))`


### 7.2 Value Nodes: General Evaluation Pattern

For pure nodes, `⟦n⟧_v` is defined as an SMT expression over input terms.

#### (V-Con)
For constant nodes (prototype subset: `ConI`, `ConL`, `ConP`, and boolean constants):
`⟦ConK(c)⟧_v = c`

#### (V-Parm)
For parameter node `Parm(i)`:
`⟦Parm(i)⟧_v = arg_i` (fresh SMT variable)

#### (V-Arith)
Examples:
- `⟦AddI(x,y)⟧_v = bvadd(⟦x⟧, ⟦y⟧)`
- `⟦SubL(x,y)⟧_v = bvsub(⟦x⟧, ⟦y⟧)`
- `⟦AndI(x,y)⟧_v = bvand(⟦x⟧, ⟦y⟧)`
- `⟦XorL(x,y)⟧_v = bvxor(⟦x⟧, ⟦y⟧)`

#### (V-Cmp/Bool)
We normalize comparisons into `Bool` terms:
- `⟦CmpI(x,y)⟧_v` may yield an `Int32`/tri-state in C2, but for the prototype we prefer direct boolean comparators:
- Use `Bool(Cmp??(...))` patterns, or introduce:
  - `⟦CmpEq(x,y)⟧_v = (⟦x⟧ = ⟦y⟧)`
  - `⟦CmpLtS(x,y)⟧_v = bvslt(⟦x⟧, ⟦y⟧)` etc.

If the SoN graph uses C2-style `CmpX` + `Bool` nodes:
- define a mapping in canonicalization to a boolean term.

#### (V-Cast)
Casts are either:
- no-ops at the bit-level (e.g., narrowing handled explicitly), or
- constraints (e.g., `CastPP` conveys type info).
For the prototype:
- treat casts as identity on representation unless they affect bitwidth:
  - `ConvI2L`: `sign_extend_32_to_64(⟦x⟧)`
  - `ConvL2I`: `extract_low32(⟦x⟧)`


### 7.3 Merge Nodes: `Phi` (Values)

Let `Phi` be attached to a `Region` with control predecessors `c1..ck`. Let incoming values be `v1..vk`.

Define the selection guard `Gi = PC(ci)`.

#### (V-Phi)
`⟦Phi⟧_v = ite(G1, ⟦v1⟧,
           ite(G2, ⟦v2⟧,
           ...
           ⟦vk⟧ ...))`

Additionally, assert (or assume) `WF_Region` so the selection is unambiguous.


## 8. Translation Validation on SoN: Memory Semantics with PC and Heap Annotation

We treat memory nodes as producing a heap term `⟦m⟧_H`.

### 8.1 Memory Merge: `MergeMem` / memory-phi

If memory is represented as one unified heap `H`, then memory merges at a `Region` are handled like Phi:

#### (H-Phi)
For a merge of heaps `H1..Hk` at control preds `c1..ck`:
`H_merge = ite(PC(c1), H1,
          ite(PC(c2), H2,
          ...
          Hk ...))`

If C2 provides `MergeMem` with slices, the prototype can:
- **Option A (simplest)**: flatten slices into one heap (sound but less precise for some reorderings).
- **Option B**: model slices as a product heap `(H_slice0, H_slice1, ...)` and merge per slice.

Initial prototype typically uses Option A.


### 8.2 Allocation (`Allocate`, `AllocateArray`)

Allocation is **allowed** in the prototype.

State includes `next` allocation counter. We model allocation as:

- `ref = next`
- `next' = next + 1`
- heap after allocation may include default initialization

#### (H-AllocateObj)
If `Allocate` executes under control `c` with guard `g = PC(c)`:

Let current `(H, next)` and allocated ref `r = next`.
Let `H_init` be `H` updated with default values for a finite set of modeled fields (or left unconstrained if field defaults are handled elsewhere).

Then:
- `r_term = ite(g, next, r_poison_or_dummy)`
  In practice, to avoid undefined refs on non-taken paths, define:
  - `r_term = next` and rely on control gating; or
  - use `ite(g, next, r0)` for some fixed ref `r0` that is never observed (prototype-specific).
- `H' = ite(g, H_init(next), H)`  (heap updates only if executed)
- `next' = ite(g, next+1, next)`

For the simplest encoding:
- represent allocation nodes as returning:
  - `(r_term, H', next')`

#### (H-AllocateArray)
Similarly, `AllocateArray(len)` under guard `g`:
- allocate `r = next`
- set `Hlen[r] = len` (if modeling lengths)
- initialize elements to default
- increment `next` under guard

> **Note**: Because we are loop-free, the number of allocations is bounded by the graph, which enables later bijection/permutation-based heap equivalence.


### 8.3 Loads and Stores (`LoadX`, `StoreX`)

Memory nodes are guarded by `PC` at their controlling control node.

Assume:
- store node has inputs: control `c`, memory-in `H_in`, address/base `r`, and value `v`.
- load node has inputs: control `c`, memory-in `H_in`, and address/base `r`.

We also assume the canonicalizer provides a `FieldId` (for object fields) or index term (for arrays).

#### (H-StoreField)
Let guard `g = PC(c)`, base `r = ⟦base⟧_v`, value `val = ⟦v⟧_v`, and `fid = prop(Store).field_id`.

`H_out = ite(g, write_f(H_in, r, fid, val), H_in)`

#### (V-LoadField)
Load returns a value term, also guarded by control reachability:

`⟦Load⟧_v = ite(g, read_f(H_in, r, fid), default_val(type(Load)))`

The `default_val` is used only for untaken paths to keep terms total; it is semantically irrelevant if consumers are also control-guarded. Alternatively, use an unconstrained fresh symbol and rely on reachability constraints.

#### (H-StoreArray) / (V-LoadArray) (optional)
With index `i = ⟦idx⟧_v`:
- `H_out = ite(g, write_a(H_in, r, i, val), H_in)`
- `⟦Load⟧_v = ite(g, read_a(H_in, r, i), default_val)`


## 9. Translation Validation on SoN: Control Transfer and Return

### 9.1 Return
A `Return` node has controlling control `c`, memory-in `H_in`, and return value `v` (or none for `void`).

#### (O-Return)
Let `g = PC(c)`.

Define the program outcome as:
- `ret_val = ⟦v⟧_v` (or a unit value)
- `ret_heap = H_in`

In a well-formed loop-free fragment, there is exactly one reachable return. If multiple returns exist, the canonicalization should merge them into a single return via control/phi-style encoding, or the outcome is a guarded sum:
- `ret_val = ite(g1, v1, ite(g2, v2, ...))`
- `ret_heap = ite(g1, H1, ite(g2, H2, ...))`
and assert disjointness of return guards.


## 10. Summary: Opcode Set and Operational Semantics

This is the **initial supported opcode set** based on our assumption at Section 3. The canonicalizer may map multiple C2 opcodes into these semantic categories.

### 10.1 Control Opcodes
- `Start`: `PC(Start)=true`
- `If`: produces successor controls `IfTrue`, `IfFalse` with `PC` rules
- `IfTrue`, `IfFalse`: control projections guarded by condition
- `Goto`: forwards control
- `Region`: merges control via disjunction, with `WF_Region` disjointness constraint
- `Return`: yields final outcome (possibly merged)

### 10.2 Merge/Value Selection
- `Phi`: `ite`-selection based on predecessor reachability `PC`

### 10.3 Memory Merge
- `MergeMem` (or equivalent): heap `ite`-merge (per-slice optional later)

### 10.4 Allocation
- `Allocate`: guarded fresh ref allocation with `next` counter; heap init updates under guard
- `AllocateArray(len)` (optional): guarded fresh ref allocation + length init + element init

### 10.5 Loads/Stores (typed by suffix)
- `LoadI/LoadL/LoadP/...`: guarded `read` from heap input
- `StoreI/StoreL/StoreP/...`: guarded `write` into heap input

### 10.6 Pure Computations (representative set)
- Integer arithmetic: `AddI/SubI/MulI`, `AddL/SubL/MulL`
- Bitwise: `AndI/OrI/XorI`, `AndL/OrL/XorL`, shifts (`LShift`, `RShift`, `URShift`)
- Comparisons normalized to Bool: equality and signed/unsigned compares
- Conversions: `ConvI2L`, `ConvL2I` (and others as needed)
- Casts treated as identity unless bitwidth changes

> **Out of scope for initial prototype** (explicitly excluded by assumptions):
- loops (`Loop`, `CountedLoop`, etc.)
- calls (`Call*`)
- deopt/uncommon traps (`SafePoint`, uncommon trap nodes)
- synchronization (`Lock`, `Unlock`, monitors)
- volatiles / fences (`MemBar*`)
- concurrency/JMM reasoning
- floating-point (excluded by the **fp-free** assumption; can be added later once integer+heap fragment works)


## 11. Equivalence Notion for `suntv`

Given outcomes:
- `Outcome1 = Return(v1, H1)` (or merged)
- `Outcome2 = Return(v2, H2)`

We check equivalence under:
1) value equality for non-reference return values
2) reference equality **modulo allocation renaming** for reference returns
3) heap equivalence **modulo the same renaming**

### 11.1 Allocation Renaming (bounded, prototype)
Because graphs are loop-free, the number of allocations is bounded by the graph. Let:
- allocations in G1: `a1_0, ..., a1_{n-1}`
- allocations in G2: `a2_0, ..., a2_{m-1}`

Prototype approach: require `n = m` (or pad with dummy allocations) and introduce a permutation `ρ` over `{0..n-1}`.

Then:
- `ρ(a2_i) = a1_{perm(i)}`
- compare `v1` and `ρ(v2)` if return is ref
- compare `H1` and `H2` by reading corresponding locations under `ρ`

This avoids quantifiers and is SMT-friendly.

> Later improvement: reachable-only heap equivalence (more permissive), but requires reachability reasoning.

### 11.2 TV Query
Encode non-equivalence:
- `v1 ≠ v2` (after renaming if needed) OR
- `HeapNotEq(H1, H2, ρ)`

Ask solver for `SAT`:
- `SAT` ⇒ counterexample input + witness permutation
- `UNSAT` ⇒ validated for this fragment


## 12. Implementation Notes (high level)

### 12.1. Artifact Choice (Graph Input)

Use **IGV (Ideal Graph Visualizer) dumps** as the interchange artifact:
- C2 can dump ideal graphs read by IGV.
- Graal can also dump graphs read by IGV (future extensibility).

Parse IGV dumps into a canonical internal graph model. IGV is not the internal IR; it is a convenient serialization format.


### 12.2 Tool Interfaces (CLI)

The project provides two executables:

- **`suni [options...] GRAPH [ARGS...]`**: Interpret one graph concretely.
  `GRAPH` is a **positional** argument specifying the input IGV dump. `ARGS` are optional graph arguments. All other settings are options.

- **`suntv [options...] G1 G2`**: Validate equivalence of two graphs.
  `G1` and `G2` are **positional** arguments specifying the “before” and “after” IGV dumps. All other settings are options.


## 13. Next Steps

1) Add **deopt/uncommon trap** outcomes with a formal state reconstruction.
2) Add selective **calls/intrinsics** with trusted/uninterpreted specs.
3) Add volatiles/synchronization: requires an event-based or axiomatic layer aligned with the Java Memory Model.
