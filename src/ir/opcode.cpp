#include "suntv/ir/opcode.hpp"

#include <unordered_map>

namespace sun {

std::string OpcodeToString(Opcode op) {
  switch (op) {
    // Control
    case Opcode::Start:
      return "Start";
    case Opcode::If:
      return "If";
    case Opcode::IfTrue:
      return "IfTrue";
    case Opcode::IfFalse:
      return "IfFalse";
    case Opcode::Region:
      return "Region";
    case Opcode::Goto:
      return "Goto";
    case Opcode::Return:
      return "Return";
    case Opcode::Root:
      return "Root";

    // Constants
    case Opcode::ConI:
      return "ConI";
    case Opcode::ConL:
      return "ConL";
    case Opcode::ConP:
      return "ConP";

    // Arithmetic - Int32
    case Opcode::AddI:
      return "AddI";
    case Opcode::SubI:
      return "SubI";
    case Opcode::MulI:
      return "MulI";
    case Opcode::DivI:
      return "DivI";
    case Opcode::ModI:
      return "ModI";
    case Opcode::AbsI:
      return "AbsI";

    // Arithmetic - Int64
    case Opcode::AddL:
      return "AddL";
    case Opcode::SubL:
      return "SubL";
    case Opcode::MulL:
      return "MulL";
    case Opcode::DivL:
      return "DivL";
    case Opcode::ModL:
      return "ModL";
    case Opcode::AbsL:
      return "AbsL";

    // Bitwise - Int32
    case Opcode::AndI:
      return "AndI";
    case Opcode::OrI:
      return "OrI";
    case Opcode::XorI:
      return "XorI";
    case Opcode::LShiftI:
      return "LShiftI";
    case Opcode::RShiftI:
      return "RShiftI";
    case Opcode::URShiftI:
      return "URShiftI";

    // Bitwise - Int64
    case Opcode::AndL:
      return "AndL";
    case Opcode::OrL:
      return "OrL";
    case Opcode::XorL:
      return "XorL";
    case Opcode::LShiftL:
      return "LShiftL";
    case Opcode::RShiftL:
      return "RShiftL";
    case Opcode::URShiftL:
      return "URShiftL";

    // Comparison
    case Opcode::CmpI:
      return "CmpI";
    case Opcode::CmpL:
      return "CmpL";
    case Opcode::CmpP:
      return "CmpP";
    case Opcode::CmpU:
      return "CmpU";
    case Opcode::CmpUL:
      return "CmpUL";
    case Opcode::Bool:
      return "Bool";

    // Casts/Conversions
    case Opcode::ConvI2L:
      return "ConvI2L";
    case Opcode::ConvL2I:
      return "ConvL2I";
    case Opcode::CastII:
      return "CastII";
    case Opcode::CastLL:
      return "CastLL";
    case Opcode::CastPP:
      return "CastPP";
    case Opcode::CastX2P:
      return "CastX2P";
    case Opcode::CastP2X:
      return "CastP2X";

    // Conditional move
    case Opcode::CMoveI:
      return "CMoveI";
    case Opcode::CMoveL:
      return "CMoveL";
    case Opcode::CMoveP:
      return "CMoveP";

    // Memory - Loads
    case Opcode::LoadB:
      return "LoadB";
    case Opcode::LoadUB:
      return "LoadUB";
    case Opcode::LoadS:
      return "LoadS";
    case Opcode::LoadUS:
      return "LoadUS";
    case Opcode::LoadI:
      return "LoadI";
    case Opcode::LoadL:
      return "LoadL";
    case Opcode::LoadP:
      return "LoadP";
    case Opcode::LoadN:
      return "LoadN";

    // Memory - Stores
    case Opcode::StoreB:
      return "StoreB";
    case Opcode::StoreC:
      return "StoreC";
    case Opcode::StoreI:
      return "StoreI";
    case Opcode::StoreL:
      return "StoreL";
    case Opcode::StoreP:
      return "StoreP";
    case Opcode::StoreN:
      return "StoreN";

    // Memory - Merge
    case Opcode::MergeMem:
      return "MergeMem";

    // Allocation
    case Opcode::Allocate:
      return "Allocate";
    case Opcode::AllocateArray:
      return "AllocateArray";

    // Parameters
    case Opcode::Parm:
      return "Parm";

    // Merge/Phi
    case Opcode::Phi:
      return "Phi";

    // Projection
    case Opcode::Proj:
      return "Proj";

    // Address calculation
    case Opcode::AddP:
      return "AddP";

    // Unknown
    case Opcode::Unknown:
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
    m["Start"] = Opcode::Start;
    m["If"] = Opcode::If;
    m["IfTrue"] = Opcode::IfTrue;
    m["IfFalse"] = Opcode::IfFalse;
    m["Region"] = Opcode::Region;
    m["Goto"] = Opcode::Goto;
    m["Return"] = Opcode::Return;
    m["Root"] = Opcode::Root;

    // Constants
    m["ConI"] = Opcode::ConI;
    m["ConL"] = Opcode::ConL;
    m["ConP"] = Opcode::ConP;

    // Arithmetic - Int32
    m["AddI"] = Opcode::AddI;
    m["SubI"] = Opcode::SubI;
    m["MulI"] = Opcode::MulI;
    m["DivI"] = Opcode::DivI;
    m["ModI"] = Opcode::ModI;
    m["AbsI"] = Opcode::AbsI;

    // Arithmetic - Int64
    m["AddL"] = Opcode::AddL;
    m["SubL"] = Opcode::SubL;
    m["MulL"] = Opcode::MulL;
    m["DivL"] = Opcode::DivL;
    m["ModL"] = Opcode::ModL;
    m["AbsL"] = Opcode::AbsL;

    // Bitwise - Int32
    m["AndI"] = Opcode::AndI;
    m["OrI"] = Opcode::OrI;
    m["XorI"] = Opcode::XorI;
    m["LShiftI"] = Opcode::LShiftI;
    m["RShiftI"] = Opcode::RShiftI;
    m["URShiftI"] = Opcode::URShiftI;

    // Bitwise - Int64
    m["AndL"] = Opcode::AndL;
    m["OrL"] = Opcode::OrL;
    m["XorL"] = Opcode::XorL;
    m["LShiftL"] = Opcode::LShiftL;
    m["RShiftL"] = Opcode::RShiftL;
    m["URShiftL"] = Opcode::URShiftL;

    // Comparison
    m["CmpI"] = Opcode::CmpI;
    m["CmpL"] = Opcode::CmpL;
    m["CmpP"] = Opcode::CmpP;
    m["CmpU"] = Opcode::CmpU;
    m["CmpUL"] = Opcode::CmpUL;
    m["Bool"] = Opcode::Bool;

    // Casts/Conversions
    m["ConvI2L"] = Opcode::ConvI2L;
    m["ConvL2I"] = Opcode::ConvL2I;
    m["CastII"] = Opcode::CastII;
    m["CastLL"] = Opcode::CastLL;
    m["CastPP"] = Opcode::CastPP;
    m["CastX2P"] = Opcode::CastX2P;
    m["CastP2X"] = Opcode::CastP2X;

    // Conditional move
    m["CMoveI"] = Opcode::CMoveI;
    m["CMoveL"] = Opcode::CMoveL;
    m["CMoveP"] = Opcode::CMoveP;

    // Memory - Loads
    m["LoadB"] = Opcode::LoadB;
    m["LoadUB"] = Opcode::LoadUB;
    m["LoadS"] = Opcode::LoadS;
    m["LoadUS"] = Opcode::LoadUS;
    m["LoadI"] = Opcode::LoadI;
    m["LoadL"] = Opcode::LoadL;
    m["LoadP"] = Opcode::LoadP;
    m["LoadN"] = Opcode::LoadN;

    // Memory - Stores
    m["StoreB"] = Opcode::StoreB;
    m["StoreC"] = Opcode::StoreC;
    m["StoreI"] = Opcode::StoreI;
    m["StoreL"] = Opcode::StoreL;
    m["StoreP"] = Opcode::StoreP;
    m["StoreN"] = Opcode::StoreN;

    // Memory - Merge
    m["MergeMem"] = Opcode::MergeMem;

    // Allocation
    m["Allocate"] = Opcode::Allocate;
    m["AllocateArray"] = Opcode::AllocateArray;

    // Parameters
    m["Parm"] = Opcode::Parm;

    // Merge/Phi
    m["Phi"] = Opcode::Phi;

    // Projection
    m["Proj"] = Opcode::Proj;

    // Address calculation
    m["AddP"] = Opcode::AddP;

    return m;
  }();

  auto it = map.find(name);
  if (it != map.end()) {
    return it->second;
  }
  return Opcode::Unknown;
}

bool IsControl(Opcode op) {
  switch (op) {
    case Opcode::Start:
    case Opcode::If:
    case Opcode::IfTrue:
    case Opcode::IfFalse:
    case Opcode::Region:
    case Opcode::Goto:
    case Opcode::Return:
    case Opcode::Root:
      return true;
    default:
      return false;
  }
}

bool IsPure(Opcode op) {
  // Pure operations have no side effects
  switch (op) {
    // Constants
    case Opcode::ConI:
    case Opcode::ConL:
    case Opcode::ConP:
    // Arithmetic
    case Opcode::AddI:
    case Opcode::SubI:
    case Opcode::MulI:
    case Opcode::DivI:
    case Opcode::ModI:
    case Opcode::AbsI:
    case Opcode::AddL:
    case Opcode::SubL:
    case Opcode::MulL:
    case Opcode::DivL:
    case Opcode::ModL:
    case Opcode::AbsL:
    // Bitwise
    case Opcode::AndI:
    case Opcode::OrI:
    case Opcode::XorI:
    case Opcode::LShiftI:
    case Opcode::RShiftI:
    case Opcode::URShiftI:
    case Opcode::AndL:
    case Opcode::OrL:
    case Opcode::XorL:
    case Opcode::LShiftL:
    case Opcode::RShiftL:
    case Opcode::URShiftL:
    // Comparison
    case Opcode::CmpI:
    case Opcode::CmpL:
    case Opcode::CmpP:
    case Opcode::CmpU:
    case Opcode::CmpUL:
    case Opcode::Bool:
    // Casts
    case Opcode::ConvI2L:
    case Opcode::ConvL2I:
    case Opcode::CastII:
    case Opcode::CastLL:
    case Opcode::CastPP:
    case Opcode::CastX2P:
    case Opcode::CastP2X:
    // Conditional move
    case Opcode::CMoveI:
    case Opcode::CMoveL:
    case Opcode::CMoveP:
    // Address calculation (pure computation)
    case Opcode::AddP:
      return true;
    default:
      return false;
  }
}

bool IsMemory(Opcode op) {
  switch (op) {
    // Loads
    case Opcode::LoadB:
    case Opcode::LoadUB:
    case Opcode::LoadS:
    case Opcode::LoadUS:
    case Opcode::LoadI:
    case Opcode::LoadL:
    case Opcode::LoadP:
    case Opcode::LoadN:
    // Stores
    case Opcode::StoreB:
    case Opcode::StoreC:
    case Opcode::StoreI:
    case Opcode::StoreL:
    case Opcode::StoreP:
    case Opcode::StoreN:
    // Merge
    case Opcode::MergeMem:
    // Allocation
    case Opcode::Allocate:
    case Opcode::AllocateArray:
      return true;
    default:
      return false;
  }
}

bool IsMerge(Opcode op) {
  return op == Opcode::Phi || op == Opcode::Region || op == Opcode::MergeMem;
}

}  // namespace sun
