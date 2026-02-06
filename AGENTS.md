# Sun — Agent Guide

Goal: implement `suni` (concrete SoN interpreter) and `suntv` (translation validator) in C++ with Bitwuzla.

+ Knowledge Background: Optimizing JIT Compilers, Translation Validation, SMT (Bit-Vector Theory)
+ Implementation Language: C++ 20
+ Primary Source Directory: ./src

## Project Overview

The Sun project provides tools for translation validation of HotSpot C2 Sea-of-Nodes (SoN) graphs. As a first verion, we assume the input SoNs satisfy the following restrictions:
- fp-free
- loop-free
- call-free
- deopt-free
- volatile-free
- synchronization-free
- exception allowed
- allocation allowed

The formal semantics and methodology live in **[DOCS.md](./docs.md)** and **[NODES.md](./NODES.md)**.

## Toolchain Overview

The Sun toolchain consists of two main command-line tools, based on SoNs dumped in **IGV (Ideal Graph Visualizer)** format.

| Tool | Role |
|------|------|
| `suni` | concretely evaluates a *restricted* Sea-of-Nodes graph and produces the **outcome** (return/exception value + heap + side conditions) |
| `suntv` | checks equivalence of two graphs (G1 before / G2 after) by encoding **non-equivalence** to SMT and asking Bitwuzla for a counterexample |

Interpret a single graph:

```bash
./build/bin/suni [options...] GRAPH [ARGS...]
```

`GRAPH` and `ARGS` are **positional** arguments (input IGV dump and input to it). Everything else is an option.

Validate two graphs for equivalence:

```bash
./build/bin/suntv [options...] G1 G2
```

`G1` and `G2` are **positional** arguments. Everything else is an option.

Goto **[README.md](./README.md)** for detailed usage instructions.

## Build system

- Use **CMake**.
- Target C++ standard: **C++20** preferred (C++17 acceptable if already chosen).
- Integrate **Bitwuzla** as a dependency (system install or FetchContent).

Build commands (expected):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
```

If adding new external dependencies, keep them minimal and document them in README.

## Repository Structure

High-level modules:
- `include/suntv/igv/` — IGV parsing + canonicalization
- `include/suntv/ir/` — canonical graph IR
- `include/suntv/sem/` — symbolic operational semantics (PC + heap)
- `include/suntv/smt/` — Bitwuzla wrapper (sorts, terms, solver utilities)
- `include/suntv/tv/` — translation validation logic (equivalence, renaming)
- `tools/suni.cpp` — CLI entrypoint
- `tools/suntv.cpp` — CLI entrypoint
- `tests/` — unit + integration tests with small IGV fixtures

If file names or directories differ, keep these boundaries intact conceptually.

## Coding Standards

Prefer Google's C++ Style Guide as a baseline for formatting and conventions, and for naming.

Suggested C++ patterns:
- Use `std::variant` / `std::optional` for structured results.
- Avoid exceptions across module boundaries unless the project already uses exceptions consistently.

Formatting:
- Keep files clang-format friendly; avoid overly clever templates.

## Semantic Implementation Rules

Below is a summary. See **[DOCS.md](./docs.md)** for full formalism.

### PC (path condition) encoding
- Compute `PC` for control nodes:
  - `PC(Start) = true`
  - `IfTrue/IfFalse` are guarded by condition and predecessor PC
  - `Region` is disjunction of predecessor PCs
- Enforce or assume **disjointness** for structured merges (Region predecessors mutually exclusive):
  - Either assert a `WF_Region` constraint in SMT, or reject graphs that violate.

### Phi encoding
- Encode Phi as nested `ite(PC(pred_i), v_i, ...)`.
- Ensure the same predecessor ordering is used consistently.

### Heap encoding
- Use a functional heap model:
  - `read_f` / `write_f` for fields
  - optionally arrays (`read_a` / `write_a`) and lengths
- Memory nodes are guarded by `PC` of their controlling control:
  - Store updates: `H_out = ite(g, write(...), H_in)`
  - Load returns: `ite(g, read(...), default_val)` (or fresh unconstrained value)
- Treat `MergeMem` as heap-phi (unified heap for the prototype unless slices are implemented explicitly).

### Allocation
- Allocation allowed. Model with a `next_id` counter:
  - allocate under guard `g`: `ref = next_id`, `next_id' = ite(g, next_id+1, next_id)`
  - heap initialization updates guarded by `g`
- Keep allocation count bounded by graph structure (loop-free assumption).

### FP-free
- Reject floating-point opcodes/constants (e.g., ConF/ConD, AddF/AddD, etc.) in the prototype.
- Do not silently coerce FP into bitvectors.

## Translation validation Logic

Below is a summary. See **[DOCS.md](./docs.md)** for full formalism.

### Non-equivalence query pattern
- Build symbolic outcomes for both graphs: `(v1, H1, constraints1)` and `(v2, H2, constraints2)`.
- Encode **non-equivalence** as a disjunction:
  - different outcome kind, or
  - return values differ (after renaming if ref), or
  - heaps differ under renaming
- Query Bitwuzla:
  - SAT => emit counterexample model (inputs + optional renaming witness)
  - UNSAT => validated for the fragment

### Heap equivalence and renaming (prototype)
- Use a bounded **permutation/bijection** over the finite set of fresh allocations.
- Implement as:
  - `perm[i]` an integer index or as explicit pairwise-distinct mapping constraints.
- Apply renaming consistently to:
  - returned refs
  - heap reads for comparison

Avoid quantifiers in the prototype.

## IGV parsing requirements

- Preserve:
  - node IDs (stable)
  - opcode/kind
  - input edges (distinguish value/control/memory as needed)
  - key properties required for semantics (constants, field IDs, offsets, etc.)
- Drop/ignore visualization-only metadata.
- Canonicalize to a stable internal representation:
  - stable ordering of inputs and predecessor lists
  - normalized boolean conditions (Cmp+Bool patterns mapped if needed)

If parsing encounters unknown node kinds:
- reject with a clear message unless the node can be proven semantic-noop in this fragment.

## Performance and Solver Hygiene

- Prefer DAG-shaped term construction (memoize node terms).
- Avoid rebuilding identical subterms.
- Keep constraints minimal:
  - only assert well-formedness needed for soundness
  - avoid huge `ite` chains when possible (but acceptable in loop-free baseline)
- Provide `--dump-smt` option for debugging.
- Provide `--timeout` option (milliseconds) for solver.
- Enable SMT parallelism.

## Testing – TDD Approach (MANDATORY)

ALWAYS follow a strict Test-Driven Development discipline.

### Required workflow

1. Write **five failing tests** that expose the bug or demonstrate the desired behavior
2. Run the tests **one by one** to confirm they fail
3. Implement the fix or feature
4. Run the tests **one by one** to confirm they pass
5. Add the **smallest additional test** that covers edge cases, in the correct test directory

### Never

1. Disable failing tests
2. Modify tests to avoid triggering bugs
3. Add workarounds that bypass the real issue
4. Implement features without a test demonstrating them first

## Dependency Management

### C++
- Dependencies are managed **manually**
- Prefer header-only or standard-library-only solutions
- When introducing a new dependency:
  - Update `README.md`
  - Clearly document installation steps and versions

### Python (if used for tooling)
- Virtual environment: `./venv`
- Activate with:
  ```bash
  source venv/bin/activate
  ````

* Dependencies:
  * `requirements.txt` – runtime
  * `requirements.dev.txt` – development
* Always pin exact versions

## Best Practices

1. Use git frequently and meaningfully
2. Follow **Conventional Commits**
3. Keep `README.md`, `DOCS.md`, `NODES.md`, and `AGENTS.md` up to date
4. Fix **all compiler warnings**
5. Keep a clean, layered project structure
6. Write high-quality comments that explain *why*, not *what*

## Before Starting Work

1. Review recent history:

   ```bash
   git log [--oneline] [--stat] [--name-only] # Show brief/extended history
   git show [--summary] [--stat] [--name-only] <commit> # Show brief/extended history of a commit
   git diff <commit> <commit> # Compare two different commits
   git checkout <commit> # Checkout and inspect all the details of a commit
   ```
2. Understand existing design decisions before changing behavior
3. For large tasks, commit incrementally with clear messages

## Before Saving Changes

ALWAYS:

1. Clear all compiler warnings
2. Format code with `clang-format`
3. Ensure all tests pass (timeouts excepted)
4. Check changes with `git status`
5. Split work into small, reviewable commits
6. Use Conventional Commit messages:

```text
<type>[optional scope]: <title>

<body>

[optional footer]
```

* Title ≤ 50 characters
* Body explains intent and design impact

## Never unless with User Consent

1. Never push to the remote repository
2. Never commit broken or untested code
3. Never bypass code reviews
