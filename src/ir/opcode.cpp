#include "suntv/ir/opcode.hpp"

#include <unordered_map>

namespace sun {

std::string OpcodeToString(Opcode op) {
  switch (op) {
    // Control
    case Opcode::kStart:
      return "Start";
    case Opcode::kIf:
      return "If";
    case Opcode::kIfTrue:
      return "IfTrue";
    case Opcode::kIfFalse:
      return "IfFalse";
    case Opcode::kRegion:
      return "Region";
    case Opcode::kGoto:
      return "Goto";
    case Opcode::kReturn:
      return "Return";
    case Opcode::kRoot:
      return "Root";
    case Opcode::kHalt:
      return "Halt";

    // Constants
    case Opcode::kConI:
      return "ConI";
    case Opcode::kConL:
      return "ConL";
    case Opcode::kConP:
      return "ConP";

    // Arithmetic - Int32
    case Opcode::kAddI:
      return "AddI";
    case Opcode::kSubI:
      return "SubI";
    case Opcode::kMulI:
      return "MulI";
    case Opcode::kDivI:
      return "DivI";
    case Opcode::kModI:
      return "ModI";
    case Opcode::kAbsI:
      return "AbsI";

    // Arithmetic - Int64
    case Opcode::kAddL:
      return "AddL";
    case Opcode::kSubL:
      return "SubL";
    case Opcode::kMulL:
      return "MulL";
    case Opcode::kDivL:
      return "DivL";
    case Opcode::kModL:
      return "ModL";
    case Opcode::kAbsL:
      return "AbsL";

    // Bitwise - Int32
    case Opcode::kAndI:
      return "AndI";
    case Opcode::kOrI:
      return "OrI";
    case Opcode::kXorI:
      return "XorI";
    case Opcode::kLShiftI:
      return "LShiftI";
    case Opcode::kRShiftI:
      return "RShiftI";
    case Opcode::kURShiftI:
      return "URShiftI";

    // Bitwise - Int64
    case Opcode::kAndL:
      return "AndL";
    case Opcode::kOrL:
      return "OrL";
    case Opcode::kXorL:
      return "XorL";
    case Opcode::kLShiftL:
      return "LShiftL";
    case Opcode::kRShiftL:
      return "RShiftL";
    case Opcode::kURShiftL:
      return "URShiftL";

    // Comparison
    case Opcode::kCmpI:
      return "CmpI";
    case Opcode::kCmpL:
      return "CmpL";
    case Opcode::kCmpP:
      return "CmpP";
    case Opcode::kCmpU:
      return "CmpU";
    case Opcode::kCmpUL:
      return "CmpUL";
    case Opcode::kBool:
      return "Bool";

    // Casts/Conversions
    case Opcode::kConvI2L:
      return "ConvI2L";
    case Opcode::kConvL2I:
      return "ConvL2I";
    case Opcode::kConv2B:
      return "Conv2B";
    case Opcode::kCastII:
      return "CastII";
    case Opcode::kCastLL:
      return "CastLL";
    case Opcode::kCastPP:
      return "CastPP";
    case Opcode::kCastX2P:
      return "CastX2P";
    case Opcode::kCastP2X:
      return "CastP2X";

    // Conditional move
    case Opcode::kCMoveI:
      return "CMoveI";
    case Opcode::kCMoveL:
      return "CMoveL";
    case Opcode::kCMoveP:
      return "CMoveP";

    // Memory - Loads
    case Opcode::kLoadB:
      return "LoadB";
    case Opcode::kLoadUB:
      return "LoadUB";
    case Opcode::kLoadS:
      return "LoadS";
    case Opcode::kLoadUS:
      return "LoadUS";
    case Opcode::kLoadI:
      return "LoadI";
    case Opcode::kLoadL:
      return "LoadL";
    case Opcode::kLoadP:
      return "LoadP";
    case Opcode::kLoadN:
      return "LoadN";

    // Memory - Stores
    case Opcode::kStoreB:
      return "StoreB";
    case Opcode::kStoreC:
      return "StoreC";
    case Opcode::kStoreI:
      return "StoreI";
    case Opcode::kStoreL:
      return "StoreL";
    case Opcode::kStoreP:
      return "StoreP";
    case Opcode::kStoreN:
      return "StoreN";

    // Memory - Merge
    case Opcode::kMergeMem:
      return "MergeMem";

    // Allocation
    case Opcode::kAllocate:
      return "Allocate";
    case Opcode::kAllocateArray:
      return "AllocateArray";

    // Parameters
    case Opcode::kParm:
      return "Parm";

    // Merge/Phi
    case Opcode::kPhi:
      return "Phi";

    // Projection
    case Opcode::kProj:
      return "Proj";

    // Address calculation
    case Opcode::kAddP:
      return "AddP";

    // Runtime/Optimization markers
    case Opcode::kSafePoint:
      return "SafePoint";
    case Opcode::kOpaque1:
      return "Opaque1";
    case Opcode::kParsePredicate:
      return "ParsePredicate";
    case Opcode::kThreadLocal:
      return "ThreadLocal";
    case Opcode::kCallStaticJava:
      return "CallStaticJava";

    // Unknown
    case Opcode::kUnknown:
      return "Unknown";

    default:
      return "Unknown";
  }
}

Opcode StringToOpcode(const std::string& name) {
  // Build static map on first call
  static std::unordered_map<std::string, Opcode> map = []() {
    std::unordered_map<std::string, Opcode> m;
    // Control
    m["Start"] = Opcode::kStart;
    m["StartOSR"] = Opcode::kStart;  // OSR (On-Stack Replacement) start
    m["If"] = Opcode::kIf;
    m["IfTrue"] = Opcode::kIfTrue;
    m["IfFalse"] = Opcode::kIfFalse;
    m["Region"] = Opcode::kRegion;
    m["Goto"] = Opcode::kGoto;
    m["Return"] = Opcode::kReturn;
    m["Root"] = Opcode::kRoot;
    m["Halt"] = Opcode::kHalt;

    // Constants
    m["ConI"] = Opcode::kConI;
    m["ConL"] = Opcode::kConL;
    m["ConP"] = Opcode::kConP;
    m["Con"] = Opcode::kConI;  // Generic constant (assume int for now)

    // Arithmetic - Int32
    m["AddI"] = Opcode::kAddI;
    m["SubI"] = Opcode::kSubI;
    m["MulI"] = Opcode::kMulI;
    m["DivI"] = Opcode::kDivI;
    m["ModI"] = Opcode::kModI;
    m["AbsI"] = Opcode::kAbsI;

    // Arithmetic - Int64
    m["AddL"] = Opcode::kAddL;
    m["SubL"] = Opcode::kSubL;
    m["MulL"] = Opcode::kMulL;
    m["DivL"] = Opcode::kDivL;
    m["ModL"] = Opcode::kModL;
    m["AbsL"] = Opcode::kAbsL;

    // Bitwise - Int32
    m["AndI"] = Opcode::kAndI;
    m["OrI"] = Opcode::kOrI;
    m["XorI"] = Opcode::kXorI;
    m["LShiftI"] = Opcode::kLShiftI;
    m["RShiftI"] = Opcode::kRShiftI;
    m["URShiftI"] = Opcode::kURShiftI;

    // Bitwise - Int64
    m["AndL"] = Opcode::kAndL;
    m["OrL"] = Opcode::kOrL;
    m["XorL"] = Opcode::kXorL;
    m["LShiftL"] = Opcode::kLShiftL;
    m["RShiftL"] = Opcode::kRShiftL;
    m["URShiftL"] = Opcode::kURShiftL;

    // Comparison
    m["CmpI"] = Opcode::kCmpI;
    m["CmpL"] = Opcode::kCmpL;
    m["CmpP"] = Opcode::kCmpP;
    m["CmpU"] = Opcode::kCmpU;
    m["CmpUL"] = Opcode::kCmpUL;
    m["Bool"] = Opcode::kBool;

    // Casts/Conversions
    m["ConvI2L"] = Opcode::kConvI2L;
    m["ConvL2I"] = Opcode::kConvL2I;
    m["Conv2B"] = Opcode::kConv2B;
    m["CastII"] = Opcode::kCastII;
    m["CastLL"] = Opcode::kCastLL;
    m["CastPP"] = Opcode::kCastPP;
    m["CastX2P"] = Opcode::kCastX2P;
    m["CastP2X"] = Opcode::kCastP2X;

    // Conditional move
    m["CMoveI"] = Opcode::kCMoveI;
    m["CMoveL"] = Opcode::kCMoveL;
    m["CMoveP"] = Opcode::kCMoveP;

    // Memory - Loads
    m["LoadB"] = Opcode::kLoadB;
    m["LoadUB"] = Opcode::kLoadUB;
    m["LoadS"] = Opcode::kLoadS;
    m["LoadUS"] = Opcode::kLoadUS;
    m["LoadI"] = Opcode::kLoadI;
    m["LoadL"] = Opcode::kLoadL;
    m["LoadP"] = Opcode::kLoadP;
    m["LoadN"] = Opcode::kLoadN;

    // Memory - Stores
    m["StoreB"] = Opcode::kStoreB;
    m["StoreC"] = Opcode::kStoreC;
    m["StoreI"] = Opcode::kStoreI;
    m["StoreL"] = Opcode::kStoreL;
    m["StoreP"] = Opcode::kStoreP;
    m["StoreN"] = Opcode::kStoreN;

    // Memory - Merge
    m["MergeMem"] = Opcode::kMergeMem;

    // Allocation
    m["Allocate"] = Opcode::kAllocate;
    m["AllocateArray"] = Opcode::kAllocateArray;

    // Parameters
    m["Parm"] = Opcode::kParm;

    // Merge/Phi
    m["Phi"] = Opcode::kPhi;

    // Projection
    m["Proj"] = Opcode::kProj;

    // Address calculation
    m["AddP"] = Opcode::kAddP;

    // Runtime/Optimization markers
    m["SafePoint"] = Opcode::kSafePoint;
    m["Opaque1"] = Opcode::kOpaque1;
    m["ParsePredicate"] = Opcode::kParsePredicate;
    m["ThreadLocal"] = Opcode::kThreadLocal;
    m["CallStaticJava"] = Opcode::kCallStaticJava;

    return m;
  }();

  auto it = map.find(name);
  if (it != map.end()) {
    return it->second;
  }
  return Opcode::kUnknown;
}

bool IsControl(Opcode op) {
  switch (op) {
    case Opcode::kStart:
    case Opcode::kIf:
    case Opcode::kIfTrue:
    case Opcode::kIfFalse:
    case Opcode::kRegion:
    case Opcode::kGoto:
    case Opcode::kReturn:
    case Opcode::kRoot:
    case Opcode::kHalt:
    case Opcode::kSafePoint:
      return true;
    default:
      return false;
  }
}

bool IsPure(Opcode op) {
  // Pure operations have no side effects
  switch (op) {
    // Constants
    case Opcode::kConI:
    case Opcode::kConL:
    case Opcode::kConP:
    // Arithmetic
    case Opcode::kAddI:
    case Opcode::kSubI:
    case Opcode::kMulI:
    case Opcode::kDivI:
    case Opcode::kModI:
    case Opcode::kAbsI:
    case Opcode::kAddL:
    case Opcode::kSubL:
    case Opcode::kMulL:
    case Opcode::kDivL:
    case Opcode::kModL:
    case Opcode::kAbsL:
    // Bitwise
    case Opcode::kAndI:
    case Opcode::kOrI:
    case Opcode::kXorI:
    case Opcode::kLShiftI:
    case Opcode::kRShiftI:
    case Opcode::kURShiftI:
    case Opcode::kAndL:
    case Opcode::kOrL:
    case Opcode::kXorL:
    case Opcode::kLShiftL:
    case Opcode::kRShiftL:
    case Opcode::kURShiftL:
    // Comparison
    case Opcode::kCmpI:
    case Opcode::kCmpL:
    case Opcode::kCmpP:
    case Opcode::kCmpU:
    case Opcode::kCmpUL:
    case Opcode::kBool:
    // Casts
    case Opcode::kConvI2L:
    case Opcode::kConvL2I:
    case Opcode::kCastII:
    case Opcode::kCastLL:
    case Opcode::kCastPP:
    case Opcode::kCastX2P:
    case Opcode::kCastP2X:
    // Conditional move
    case Opcode::kCMoveI:
    case Opcode::kCMoveL:
    case Opcode::kCMoveP:
    // Address calculation (pure computation)
    case Opcode::kAddP:
      return true;
    default:
      return false;
  }
}

bool IsMemory(Opcode op) {
  switch (op) {
    // Loads
    case Opcode::kLoadB:
    case Opcode::kLoadUB:
    case Opcode::kLoadS:
    case Opcode::kLoadUS:
    case Opcode::kLoadI:
    case Opcode::kLoadL:
    case Opcode::kLoadP:
    case Opcode::kLoadN:
    // Stores
    case Opcode::kStoreB:
    case Opcode::kStoreC:
    case Opcode::kStoreI:
    case Opcode::kStoreL:
    case Opcode::kStoreP:
    case Opcode::kStoreN:
    // Merge
    case Opcode::kMergeMem:
    // Allocation
    case Opcode::kAllocate:
    case Opcode::kAllocateArray:
      return true;
    default:
      return false;
  }
}

bool IsMerge(Opcode op) {
  return op == Opcode::kPhi || op == Opcode::kRegion || op == Opcode::kMergeMem;
}

}  // namespace sun
