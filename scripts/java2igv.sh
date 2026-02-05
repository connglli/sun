#!/usr/bin/env bash
#
# Script to generate IGV XML dumps from Java source files.
#
# This script compiles Java programs and runs them with JVM flags to generate
# Ideal Graph Visualizer (IGV) XML dumps of the C2 compiler's Sea-of-Nodes IR.
#
# Requirements:
#   - Debug or FastDebug build of OpenJDK (for -XX:+PrintIdeal)
#   - OR GraalVM (which has better IGV dump support)
#
# Usage: ./java2igv.sh <JavaFile.java> [output_dir] [method_name]

set -eu

# Configuration
JAVA_BIN=${JAVA_BIN:-"java"}
JAVAC_BIN=${JAVAC_BIN:-"javac"}

# Parse arguments
if [ $# -lt 1 ]; then
    cat <<EOF
Usage: $0 <JavaFile.java> [output_dir] [method_name]

Arguments:
  JavaFile.java   : Java source file to compile
  output_dir      : Directory for output files (default: current directory)
  method_name     : Method to compile (default: compute)

Example:
  $0 Fibonacci.java ../fixtures/igv compute

Environment Variables:
  JAVA_BIN        : Path to java binary (default: java)
  JAVAC_BIN       : Path to javac binary (default: javac)

Note: This script requires a debug/fastdebug JDK build or GraalVM.
For standard OpenJDK, you'll need to build from source with:
  bash configure --with-debug-level=fastdebug
  make images

Alternatively, download a pre-built debug JDK from:
  https://jdk.java.net/ (look for debug symbols)
EOF
    exit 1
fi

JAVA_FILE="$1"
OUTPUT_DIR="${2:-.}"
METHOD_NAME="${3:-compute}"

# Validate input
if [ ! -f "$JAVA_FILE" ]; then
    echo "Error: File not found: $JAVA_FILE"
    exit 1
fi

CLASS_NAME=$(basename "$JAVA_FILE" .java)
JAVA_DIR=$(dirname "$JAVA_FILE")

echo "=== Java to IGV Compiler ==="
echo "Source:  $JAVA_FILE"
echo "Class:   $CLASS_NAME"
echo "Method:  $METHOD_NAME"
echo "Output:  $OUTPUT_DIR"
echo

# Check JVM type
JVM_INFO=$($JAVA_BIN -version 2>&1 | head -3)
echo "JVM Info:"
echo "$JVM_INFO"
echo

# Step 1: Compile
echo "[-1/4] Compiling Java source..."
cd "$JAVA_DIR"
$JAVAC_BIN "$CLASS_NAME.java"
echo "✓ Compiled successfully"
echo

# Step 2: Test if we have debug JVM support
echo "[2/4] Testing JVM capabilities..."

# Test for PrintIdeal support (debug JVM only)
if $JAVA_BIN -XX:+UnlockDiagnosticVMOptions -XX:+PrintIdeal -version 2>&1 | grep -q "notproduct"; then
    echo "⚠ Standard JVM detected (no PrintIdeal support)"
    echo "  You need a debug/fastdebug JDK build for IGV XML generation."
    echo
    echo "Options:"
    echo "  1. Build OpenJDK with --with-debug-level=fastdebug"
    echo "  2. Use GraalVM which has better IGV support"
    echo "  3. Download a debug JDK build"
    echo
    echo "For now, generating compilation log only..."

    # Fallback: Just compilation log
    LOG_FILE="${CLASS_NAME}_hotspot.log"
    $JAVA_BIN \
        -XX:+UnlockDiagnosticVMOptions \
        -XX:+LogCompilation \
        -XX:LogFile="$LOG_FILE" \
        -XX:CompileCommand=compileonly,${CLASS_NAME}.${METHOD_NAME} \
        -XX:-TieredCompilation \
        -XX:CompileThreshold=100 \
        $CLASS_NAME

    mkdir -p "$OUTPUT_DIR"
    mv "$LOG_FILE" "$OUTPUT_DIR/"
    echo "✓ Compilation log saved: $OUTPUT_DIR/$LOG_FILE"
    exit 0
fi

# Step 3: Generate IGV XML (debug JVM)
echo "[3/4] Generating IGV XML dump..."

IGV_FILE="${CLASS_NAME}_igv.xml"

$JAVA_BIN \
    -XX:+UnlockDiagnosticVMOptions \
    -XX:+PrintIdeal \
    -XX:PrintIdealGraphLevel=2 \
    -XX:PrintIdealGraphFile="$IGV_FILE" \
    -XX:CompileCommand=compileonly,${CLASS_NAME}.${METHOD_NAME} \
    -XX:-TieredCompilation \
    -XX:CompileThreshold=100 \
    -XX:+PrintCompilation \
    $CLASS_NAME

if [ ! -f "$IGV_FILE" ]; then
    echo "Error: IGV XML not generated (method might not have compiled)"
    echo "Try increasing warmup iterations in your Java program"
    exit 1
fi

echo "✓ IGV XML generated: $IGV_FILE"
echo

# Step 4: Organize output
echo "[4/4] Organizing output..."
mkdir -p "$OUTPUT_DIR"
mv "$IGV_FILE" "$OUTPUT_DIR/${CLASS_NAME}.xml"

echo
echo "=== SUCCESS ==="
echo "IGV graph: $OUTPUT_DIR/${CLASS_NAME}.xml"
echo
echo "You can now use this with suni:"
echo "  ./build/bin/suni $OUTPUT_DIR/${CLASS_NAME}.xml <args...>"
