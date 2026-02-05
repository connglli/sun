# Java Algorithm Test Suite

This directory contains Java implementations of classic algorithms for testing the Sun interpreter with real-world code patterns including loops and complex control flow.

## Java Source Files (`java/`)

1. **Fibonacci.java** - Iterative Fibonacci (loop with phi nodes)
2. **Factorial.java** - Iterative factorial (simple loop)
3. **GCD.java** - Euclidean algorithm (while loop, modulo)
4. **Power.java** - Power function (iterative multiplication)
5. **ArraySum.java** - Sum array elements (array allocation + loop)
6. **LinearSearch.java** - Linear search (array + early return)
7. **BinarySearch.java** - Binary search (complex control flow)
8. **BubbleSort.java** - Bubble sort (nested loops + array mutations)
9. **MatrixMultiply.java** - Matrix multiplication (triple nested loops + 2D arrays)
10. **IsPrime.java** - Primality test (loop with early return)

## Generating IGV Dumps

### Option 1: Debug OpenJDK Build (Recommended for C2)

To generate IGV XML dumps, you need a debug or fastdebug build of OpenJDK:

```bash
# Build OpenJDK with fastdebug
git clone https://github.com/openjdk/jdk
cd jdk
bash configure --with-debug-level=fastdebug
make images

# Use the debug JVM
export JAVA_BIN=./build/linux-x86_64-server-fastdebug/images/jdk/bin/java
export JAVAC_BIN=./build/linux-x86_64-server-fastdebug/images/jdk/bin/javac

# Generate IGV dump
../../scripts/java2igv.sh Fibonacci.java ../igv compute
```

### Option 2: GraalVM (Alternative)

GraalVM has built-in support for IGV dumps:

```bash
export JAVA_BIN=/path/to/graalvm/bin/java
../../scripts/java2igv.sh Fibonacci.java ../igv compute
```

### Option 3: Manual IGV XML Creation (For Testing)

For now, we've manually created simplified IGV XML files based on the expected C2 output. These are in `../igv/` and follow the IGV XML schema.

## IGV Dumps (`igv/`)

Generated (or manually created) IGV XML files will be placed here. Each file contains the Sea-of-Nodes IR for one algorithm's `compute` method.

## Testing

Once IGV dumps are generated, test them with:

```bash
# Build the project
cd ../../..
cmake --build build

# Run interpreter on a graph
./build/bin/suni tests/fixtures/igv/Fibonacci.xml 10

# Run integration tests
./build/tests/sun_integration_tests
```

## TODO

- [ ] Set up debug OpenJDK build or GraalVM
- [ ] Generate IGV dumps for all 10 algorithms
- [ ] Create integration tests that load and execute these graphs
- [ ] Verify interpreter handles loops correctly
- [ ] Add more complex algorithms (edit distance, knapsack, dijkstra)

## Notes

- All Java programs include warm-up loops to ensure C2 compilation
- The `compute` method is the target method for IGV dumps
- Standard OpenJDK does NOT support `-XX:+PrintIdeal` (requires debug build)
- For translation validation, we'll generate "before" and "after" optimization dumps
