#pragma once
#include <map>
#include <string>
#include <vector>

#include "suntv/interp/value.hpp"

namespace sun {

using FieldID = std::string;

class ConcreteHeap {
 public:
  ConcreteHeap() : next_ref_(1) {}  // Start ref allocation at 1

  // Allocation
  Ref AllocateObject();
  Ref AllocateArray(int32_t length);

  // Field access
  Value ReadField(Ref obj, const FieldID& field) const;
  void WriteField(Ref obj, const FieldID& field, Value val);

  // Array access
  Value ReadArray(Ref arr, int32_t index) const;
  void WriteArray(Ref arr, int32_t index, Value val);
  int32_t ArrayLength(Ref arr) const;

  // Debugging
  std::string Dump() const;

 private:
  Ref next_ref_;  // Next available reference

  // Heap storage: (ref, field) -> value
  std::map<std::pair<Ref, FieldID>, Value> fields_;

  // Array storage: ref -> vector of values
  std::map<Ref, std::vector<Value>> arrays_;

  // Array lengths: ref -> length
  std::map<Ref, int32_t> array_lengths_;
};

}  // namespace sun
