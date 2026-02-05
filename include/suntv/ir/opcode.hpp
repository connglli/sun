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
  Start,
  If,
  IfTrue,
  IfFalse,
  Region,
  Goto,
  Return,
  Root,

  // Constants
  ConI,  // int32 constant
  ConL,  // int64 constant
  ConP,  // pointer/reference constant (null)

  // Arithmetic - Int32
  AddI,
  SubI,
  MulI,
  DivI,
  ModI,
  AbsI,

  // Arithmetic - Int64
  AddL,
  SubL,
  MulL,
  DivL,
  ModL,
  AbsL,

  // Bitwise - Int32
  AndI,
  OrI,
  XorI,
  LShiftI,
  RShiftI,   // Arithmetic right shift
  URShiftI,  // Logical (unsigned) right shift

  // Bitwise - Int64
  AndL,
  OrL,
  XorL,
  LShiftL,
  RShiftL,
  URShiftL,

  // Comparison
  CmpI,
  CmpL,
  CmpP,
  CmpU,   // Unsigned int32 compare
  CmpUL,  // Unsigned int64 compare
  Bool,   // Convert compare result to boolean

  // Casts/Conversions
  ConvI2L,  // Sign-extend int32 to int64
  ConvL2I,  // Truncate int64 to int32
  CastII,   // Type/range cast int32
  CastLL,   // Type/range cast int64
  CastPP,   // Type/nullness cast pointer
  CastX2P,  // Machine word to pointer
  CastP2X,  // Pointer to machine word

  // Conditional move
  CMoveI,
  CMoveL,
  CMoveP,

  // Memory - Loads
  LoadB,   // Load signed byte
  LoadUB,  // Load unsigned byte
  LoadS,   // Load signed short
  LoadUS,  // Load unsigned short
  LoadI,   // Load int32
  LoadL,   // Load int64
  LoadP,   // Load pointer/reference
  LoadN,   // Load narrow (compressed) reference

  // Memory - Stores
  StoreB,  // Store byte
  StoreC,  // Store char (16-bit)
  StoreI,  // Store int32
  StoreL,  // Store int64
  StoreP,  // Store pointer/reference
  StoreN,  // Store narrow reference

  // Memory - Merge
  MergeMem,  // Memory phi

  // Allocation
  Allocate,
  AllocateArray,

  // Parameters
  Parm,  // Method parameter

  // Merge/Phi
  Phi,  // Value phi

  // Projection
  Proj,  // Project a specific output from multi-output node

  // Address calculation
  AddP,  // Pointer/address arithmetic

  // Unknown/unsupported
  Unknown
};

/**
 * Convert opcode to string name.
 */
std::string OpcodeToString(Opcode op);

/**
 * Parse string to opcode.
 * Returns Opcode::Unknown if not recognized.
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

}  // namespace sun
