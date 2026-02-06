#pragma once

#include <string>

namespace sun {

/**
 * Opcode enumeration for Sea-of-Nodes IR.
 * Based on HotSpot C2 node types, filtered for the prototype scope:
 * - fp-free, loop-free, call-free, deopt-free, volatile-free,
 * synchronization-free
 * - exception allowed, allocation allowed
 */
enum class Opcode {
  // Control
  kStart,
  kIf,
  kIfTrue,
  kIfFalse,
  kRegion,
  kGoto,
  kReturn,
  kRoot,
  kHalt,  // Abnormal termination (e.g., unhandled exception)

  // Constants
  kConI,  // int32 constant
  kConL,  // int64 constant
  kConP,  // pointer/reference constant (null)

  // Arithmetic - Int32
  kAddI,
  kSubI,
  kMulI,
  kDivI,
  kModI,
  kAbsI,

  // Arithmetic - Int64
  kAddL,
  kSubL,
  kMulL,
  kDivL,
  kModL,
  kAbsL,

  // Bitwise - Int32
  kAndI,
  kOrI,
  kXorI,
  kLShiftI,
  kRShiftI,   // Arithmetic right shift
  kURShiftI,  // Logical (unsigned) right shift

  // Bitwise - Int64
  kAndL,
  kOrL,
  kXorL,
  kLShiftL,
  kRShiftL,
  kURShiftL,

  // Comparison
  kCmpI,
  kCmpL,
  kCmpP,
  kCmpU,   // Unsigned int32 compare
  kCmpUL,  // Unsigned int64 compare
  kBool,   // Convert compare result to boolean

  // Casts/Conversions
  kConvI2L,  // Sign-extend int32 to int64
  kConvL2I,  // Truncate int64 to int32
  kConv2B,   // Convert to boolean (any non-zero -> 1, zero -> 0)
  kCastII,   // Type/range cast int32
  kCastLL,   // Type/range cast int64
  kCastPP,   // Type/nullness cast pointer
  kCastX2P,  // Machine word to pointer
  kCastP2X,  // Pointer to machine word

  // Conditional move
  kCMoveI,
  kCMoveL,
  kCMoveP,

  // Memory - Loads
  kLoadB,   // Load signed byte
  kLoadUB,  // Load unsigned byte
  kLoadS,   // Load signed short
  kLoadUS,  // Load unsigned short
  kLoadI,   // Load int32
  kLoadL,   // Load int64
  kLoadP,   // Load pointer/reference
  kLoadN,   // Load narrow (compressed) reference

  // Memory - Stores
  kStoreB,  // Store byte
  kStoreC,  // Store char (16-bit)
  kStoreI,  // Store int32
  kStoreL,  // Store int64
  kStoreP,  // Store pointer/reference
  kStoreN,  // Store narrow reference

  // Memory - Merge
  kMergeMem,  // Memory phi

  // Allocation
  kAllocate,
  kAllocateArray,

  // Array operations
  kLoadRange,   // Load array length (array.length in Java)
  kRangeCheck,  // Array bounds check (produces index if valid)

  // Parameters
  kParm,  // Method parameter

  // Merge/Phi
  kPhi,  // Value phi

  // Projection
  kProj,  // Project a specific output from multi-output node

  // Address calculation
  kAddP,  // Pointer/address arithmetic

  // Runtime/Optimization markers
  kSafePoint,       // GC safepoint (can be skipped in interpreter)
  kOpaque1,         // Optimization barrier (pass-through in interpreter)
  kParsePredicate,  // Profile-based prediction hint (can skip)
  kThreadLocal,     // Access thread-local variable
  kCallStaticJava,  // Static method call (often uncommon_trap - skip for now)

  // Unknown/unsupported
  kUnknown
};

/**
 * Convert opcode to string name.
 */
std::string OpcodeToString(Opcode op);

/**
 * Parse string to opcode.
 * Returns Opcode::kUnknown if not recognized.
 */
Opcode StringToOpcode(const std::string& name);

/**
 * Check if opcode is a control node.
 */
bool IsControl(Opcode op);

/**
 * Check if opcode is a pure (side-effect-free) computation.
 */
bool IsPure(Opcode op);

/**
 * Check if opcode has memory effects (loads, stores, allocation).
 */
bool IsMemory(Opcode op);

/**
 * Check if opcode is a merge/phi node.
 */
bool IsMerge(Opcode op);

/**
 * Node schema classification.
 * Defines the semantic input pattern for different node categories.
 * Based on NODES.md schemas (S0-S11).
 */
enum class NodeSchema {
  kS0_Pure,     // Pure computations (AddI, MulI, etc.) - value inputs only
  kS1_Control,  // Control nodes (If, IfTrue, IfFalse, Goto) - control input +
                // optional condition
  kS2_Merge,  // Merge/Phi nodes (Phi, Region, MergeMem) - control predecessors
              // + values/states
  kS3_Load,   // Load operations - control + memory + address + properties
  kS4_Store,  // Store operations - control + memory + address + value +
              // properties
  kS5_Allocate,    // Allocation nodes - control + memory + properties
  kS6_Return,      // Return node - control + memory + optional value
  kS7_Start,       // Start node - no inputs
  kS8_Projection,  // Projection node - multi-output source
  kS9_Parameter,   // Parameter node - Start node
  kUnknown         // Unknown or unclassified schema
};

/**
 * Get the semantic schema for an opcode.
 * Determines the expected input pattern based on node type.
 */
NodeSchema GetSchema(Opcode op);

}  // namespace sun
