#!/usr/bin/env bash
#
# Batch generate IGV dumps for all Java algorithm test programs.
#
# Usage: ./generate_all_igv.sh

set -e -o pipefail

# Configuration
JAVA_BIN=${JAVA_BIN:-"/zdata/openjdks/jdk21u-dev/build/linux-x86_64-server-fastdebug/images/jdk/bin/java"}
JAVAC_BIN=${JAVAC_BIN:-"/zdata/openjdks/jdk21u-dev/build/linux-x86_64-server-fastdebug/images/jdk/bin/javac"}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAVA_DIR="$SCRIPT_DIR/../tests/fixtures/java"
IGV_DIR="$SCRIPT_DIR/../tests/fixtures/igv"

# Java programs to process
PROGRAMS=(
    "Fibonacci"
    "Factorial"
    "GCD"
    "Power"
    "ArraySum"
    "LinearSearch"
    "BinarySearch"
    "BubbleSort"
    "MatrixMultiply"
    "IsPrime"
)

echo "=== Batch IGV Generation ==="
echo "Java:   $JAVA_BIN"
echo "Javac:  $JAVAC_BIN"
echo "Source: $JAVA_DIR"
echo "Output: $IGV_DIR"
echo

# Verify JVM
if ! $JAVA_BIN -XX:+UnlockDiagnosticVMOptions -XX:+PrintIdeal -version &>/dev/null; then
    echo "Error: JVM does not support PrintIdeal (need fastdebug build)"
    exit 1
fi

mkdir -p "$IGV_DIR"
cd "$JAVA_DIR"

SUCCESS=0
FAILED=0

for PROGRAM in "${PROGRAMS[@]}"; do
    echo "[$((SUCCESS + FAILED + 1))/${#PROGRAMS[@]}] Processing $PROGRAM..."

    # Compile
    if $JAVAC_BIN "${PROGRAM}.java" 2>&1; then
        echo "  ✓ Compiled"
    else
        echo "  ✗ Compilation failed"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Generate IGV dump
    IGV_FILE="${PROGRAM}_igv.xml"
    $JAVA_BIN \
        -XX:+UnlockDiagnosticVMOptions \
        -XX:+PrintIdeal \
        -XX:PrintIdealGraphLevel=2 \
        -XX:PrintIdealGraphFile="$IGV_FILE" \
        -XX:CompileCommand=compileonly,${PROGRAM}.compute \
        -XX:-TieredCompilation \
        -XX:CompileThreshold=100 \
        -XX:+PrintCompilation \
        $PROGRAM >/dev/null 2>&1

    if [ -f "$IGV_FILE" ]; then
        SIZE=$(du -h "$IGV_FILE" | cut -f1)
        mv "$IGV_FILE" "$IGV_DIR/${PROGRAM}.xml"
        echo "  ✓ IGV dump generated ($SIZE)"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "  ✗ IGV dump failed (method might not have compiled)"
        FAILED=$((FAILED + 1))
    fi
    echo
done

echo "=== Summary ==="
echo "Success: $SUCCESS"
echo "Failed:  $FAILED"
echo
echo "IGV dumps are in: $IGV_DIR"
ls -lh "$IGV_DIR"/*.xml 2>/dev/null || echo "No XML files found"
