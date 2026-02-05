#include "suntv/interp/heap.hpp"

#include <sstream>
#include <stdexcept>

namespace sun {

Ref ConcreteHeap::AllocateObject() {
  Ref ref = next_ref_++;
  // Objects have no default initialization in this model
  return ref;
}

Ref ConcreteHeap::AllocateArray(int32_t length) {
  if (length < 0) {
    throw std::runtime_error("Negative array length");
  }
  Ref ref = next_ref_++;
  arrays_[ref] = std::vector<Value>(length, Value::MakeI32(0));  // Default init
  array_lengths_[ref] = length;
  return ref;
}

Value ConcreteHeap::ReadField(Ref obj, const FieldID& field) const {
  auto key = std::make_pair(obj, field);
  auto it = fields_.find(key);
  if (it == fields_.end()) {
    // Uninitialized field returns default (0)
    return Value::MakeI32(0);
  }
  return it->second;
}

void ConcreteHeap::WriteField(Ref obj, const FieldID& field, Value val) {
  auto key = std::make_pair(obj, field);
  fields_[key] = val;
}

Value ConcreteHeap::ReadArray(Ref arr, int32_t index) const {
  auto it = arrays_.find(arr);
  if (it == arrays_.end()) {
    throw std::runtime_error("Invalid array reference");
  }
  if (index < 0 || index >= static_cast<int32_t>(it->second.size())) {
    throw std::runtime_error("Array index out of bounds");
  }
  return it->second[index];
}

void ConcreteHeap::WriteArray(Ref arr, int32_t index, Value val) {
  auto it = arrays_.find(arr);
  if (it == arrays_.end()) {
    throw std::runtime_error("Invalid array reference");
  }
  if (index < 0 || index >= static_cast<int32_t>(it->second.size())) {
    throw std::runtime_error("Array index out of bounds");
  }
  it->second[index] = val;
}

int32_t ConcreteHeap::ArrayLength(Ref arr) const {
  auto it = array_lengths_.find(arr);
  if (it == array_lengths_.end()) {
    throw std::runtime_error("Invalid array reference");
  }
  return it->second;
}

std::string ConcreteHeap::Dump() const {
  std::ostringstream oss;
  oss << "=== Heap Dump ===" << std::endl;
  oss << "Next ref: " << next_ref_ << std::endl;

  if (!fields_.empty()) {
    oss << "Fields:" << std::endl;
    for (const auto& [key, val] : fields_) {
      oss << "  ref:" << key.first << "." << key.second << " = "
          << val.ToString() << std::endl;
    }
  }

  if (!arrays_.empty()) {
    oss << "Arrays:" << std::endl;
    for (const auto& [ref, arr] : arrays_) {
      oss << "  ref:" << ref << "[" << array_lengths_.at(ref) << "]";
      if (!arr.empty()) {
        oss << " = {";
        for (size_t i = 0; i < arr.size(); ++i) {
          if (i > 0) oss << ", ";
          oss << arr[i].ToString();
        }
        oss << "}";
      }
      oss << std::endl;
    }
  }

  oss << "=================" << std::endl;
  return oss.str();
}

}  // namespace sun
