# Sun — Translation Validation for HotSpot C2 Sea-of-Nodes

This repository contains a prototype toolchain for **translation validation (TV)** of HotSpot **C2** Sea-of-Nodes (SoN) graphs.

- **`suni`** — an SoN interpreter (concrete execution)
- **`suntv`** — a translation validator for a pair of graphs (before/after optimization)

Implementation: **C++** with **Bitwuzla** as the SMT backend.


## Scope and Assumptions

The initial prototype supports SoN graphs restricted to:

- **fp-free**
- **loop-free**
- **call-free**
- **deopt-free**
- **volatile-free**
- **synchronization-free**
- **exception allowed**
- **allocation allowed**

The modeled observable behaviors are:
- `Return(value, heap)` and `Throw(kind, heap)`.


## Input Artifact: IGV Dumps

Both tools consume graphs dumped in **IGV (Ideal Graph Visualizer)** format.

Workflow:
1. Dump a C2 graph (and later optionally Graal graphs) in IGV format.
2. Parse + canonicalize the IGV dump into an internal graph model.
3. Interpret (`suni`) or validate (`suntv`) using SMT encodings.


## Command-Line Usage

### `suni` — interpret one graph

**Positional argument**:
- `GRAPH` — path to an IGV dump file
- `ARGN` — optional arguments to the graph (if any)

Options (examples; exact set may evolve):
- `--format {igv}`: input format (default: `igv`)
- `--stats`: print internal statistics

Output:
- The concrete outcome (return/exception + heap + side conditions)

Example:
```bash
./build/bin/suni --stats path/to/graph.igv arg1 arg2 ...
```

This is an assisting tool for debugging and understanding graph behavior.

### `suntv` — validate two graphs

**Positional arguments**:
- `G1` — path to the “before” IGV dump
- `G2` — path to the “after” IGV dump

Options (examples; exact set may evolve):
- `--dump-smt FILE`: dump the generated SMT query (debug)
- `--smt-timeout MS`: solver timeout
- `--smt-parallelism N`: enable SMT parallelism with N threads
- `--stats`: print internal statistics

Output:
- Equivalence result: `SAT` (valid) or `UNSAT` (invalid)
- Counterexample model if `UNSAT`

Example:
```bash
# Validate equivalence of two graphs
./build/bin/suntv --dump-smt query.smt2 before.igv after.igv
```

> The tools treat all non-positional settings as options; input graphs are always provided positionally.


## Build

### Requirements
- CMake ≥ 3.20
- C++ compiler (C++20 recommended; C++17 may be sufficient depending on features used)
- Bitwuzla (headers + library)

### Configure and build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```


## Project organization

```text
.
├── README.md
├── DOCS.md                           # specs, semantics, methodology
├── CMakeLists.txt
├── cmake/
│   ├── FindBitwuzla.cmake            # or FetchContent logic
│   └── toolchains/                   # optional
├── include/
│   ├── suntv/
│   │   ├── igv/                      # IGV parsing + canonicalization
│   │   │   ├── igv_parser.hpp
│   │   │   └── igv_model.hpp
│   │   ├── ir/                       # canonical SoN IR
│   │   │   ├── graph.hpp
│   │   │   ├── node.hpp
│   │   │   ├── opcode.hpp
│   │   │   └── types.hpp             # stamps/types as needed
│   │   ├── sem/                      # semantics + symbolic evaluation
│   │   │   ├── outcome.hpp
│   │   │   ├── state.hpp             # heap + next_id + PC predicates
│   │   │   ├── heap.hpp
│   │   │   └── eval.hpp
│   │   ├── smt/                      # Bitwuzla wrapper + term utilities
│   │   │   ├── ctx.hpp
│   │   │   ├── sorts.hpp
│   │   │   ├── terms.hpp
│   │   │   └── solver.hpp
│   │   ├── tv/                       # translation validation (SunTV core)
│   │   │   ├── equivalence.hpp
│   │   │   ├── renaming.hpp
│   │   │   └── check.hpp
│   │   └── util/
│   │       ├── logging.hpp
│   │       └── cli.hpp
├── src/
│   ├── igv/
│   ├── ir/
│   ├── sem/
│   ├── smt/
│   ├── tv/
│   └── util/
├── tools/
│   ├── suni.cpp
│   └── suntv.cpp
├── tests/
└── docs/
```

### Module Responsibilities (high level)

- `igv/`: parse IGV dumps and canonicalize node/edge/property representation
- `ir/`: canonical graph model used by both tools
- `sem/`: operational semantics + symbolic evaluation (PC + heap model)
- `smt/`: Bitwuzla integration and SMT term helpers
- `tv/`: equivalence checking (non-equivalence query construction + model decoding)
- `tools/`: CLI front-ends (`suni`, `suntv`)


## License

MIT.
