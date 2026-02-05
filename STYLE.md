# Sun C++ Style Guide

This project follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with clarifications documented here.

## Naming Conventions

### Type Names
Use **PascalCase** for all type names:
```cpp
class Graph { };
struct Node { };
enum class Opcode { };
using NodeID = int32_t;
```

### Variable Names
Use **snake_case** with trailing underscore for private member variables:
```cpp
class Node {
 private:
  NodeID id_;              // Private member
  Opcode opcode_;          // Private member
};

void Foo() {
  int local_var = 42;      // Local variable (no trailing underscore)
  const int kConstant = 7; // Local constant (k prefix)
}
```

### Function and Method Names

#### Regular Functions and Methods
Use **PascalCase** for regular functions and methods:
```cpp
std::string OpcodeToString(Opcode op);
Opcode StringToOpcode(const std::string& name);
Node* AddNode(NodeID id, Opcode op);
void AddInput(Node* n);
std::string ToString() const;
```

#### Accessors and Mutators
Use **snake_case** for accessors (getters) and mutators (setters):
```cpp
class Node {
 public:
  // Accessors (getters)
  NodeID id() const { return id_; }
  Opcode opcode() const { return opcode_; }
  Node* input(size_t i) const;
  Property prop(const std::string& key) const;
  TypeStamp type() const { return type_; }
  size_t num_inputs() const { return inputs_.size(); }
  bool has_prop(const std::string& key) const;

  // Mutators (setters)
  void set_input(size_t i, Node* n);
  void set_prop(const std::string& key, Property value);
  void set_type(TypeStamp t) { type_ = t; }
};
```

**Rationale:** From Google C++ Style Guide:
> "Accessors and mutators (get and set functions) may be named like variables, in `snake_case`."

### Enum Values
Use **kPascalCase** (constant style with k prefix) for enum values:
```cpp
enum class Opcode {
  kStart,
  kReturn,
  kAddI,
  kConI,
  kUnknown
};
```

**Rationale:** From Google C++ Style Guide:
> "Enumerators should be named like constants, not like macros. That is, use `kEnumName` not `ENUM_NAME`."

### Constants
Use **kPascalCase** for constants:
```cpp
const int kMaxNodes = 1000;
constexpr int kDefaultTimeout = 30;
```

### Namespace Names
Use **snake_case** for namespace names:
```cpp
namespace sun {
namespace internal_helper {
}  // namespace internal_helper
}  // namespace sun
```

## Code Organization

### Header Guards
Use the format `PROJECT_PATH_FILE_H_`:
```cpp
#ifndef SUN_IR_NODE_H_
#define SUN_IR_NODE_H_
// ...
#endif  // SUN_IR_NODE_H_
```

### Include Order
1. Related header (for .cpp files)
2. C system headers
3. C++ standard library headers
4. Other libraries' headers
5. Project headers

```cpp
#include "suntv/ir/node.hpp"  // Related header

#include <unistd.h>           // C system

#include <string>             // C++ standard
#include <vector>

#include <pugixml.hpp>        // Third-party

#include "suntv/ir/graph.hpp" // Project
#include "suntv/ir/opcode.hpp"
```

## Formatting

### Indentation
- Use **2 spaces** for indentation (no tabs)
- Namespace contents are **not indented**

```cpp
namespace sun {

class Node {
 public:
  void Foo() {
    if (condition) {
      DoSomething();
    }
  }
};

}  // namespace sun
```

### Line Length
- Prefer lines under 80 characters when reasonable
- Max 100 characters for complex expressions

### Comments
```cpp
// Single-line comment style

/**
 * Multi-line documentation comment for classes/functions.
 * Explains what the entity does and how to use it.
 */
```

## Best Practices

### Const Correctness
- Mark methods `const` if they don't modify object state
- Use `const` references for parameters when not modifying

```cpp
Node* input(size_t i) const;  // const method
void AddInput(const Node* n); // const parameter (if appropriate)
```

### nullptr vs NULL
Always use `nullptr`, never `NULL` or `0`:
```cpp
Node* ptr = nullptr;  // Good
Node* ptr = NULL;     // Bad
Node* ptr = 0;        // Bad
```

### auto Keyword
Use `auto` to avoid repetition, but not when it obscures type:
```cpp
auto it = map.find(key);              // Good - obvious from context
std::map<int, Node*>::iterator it = map.find(key);  // Verbose

auto n = new Node(...);               // Bad - what type is n?
Node* n = new Node(...);              // Good - type is clear
```

### Smart Pointers
Prefer smart pointers over raw pointers for ownership:
```cpp
std::vector<std::unique_ptr<Node>> owned_nodes_;  // Owns nodes
std::vector<Node*> node_list_;                    // Non-owning pointers
```

## Example

```cpp
// suntv/ir/node.hpp
#pragma once

#include <string>
#include <vector>

namespace sun {

class Node {
 public:
  Node(NodeID id, Opcode opcode);

  // Accessors (snake_case)
  NodeID id() const { return id_; }
  Opcode opcode() const { return opcode_; }
  size_t num_inputs() const { return inputs_.size(); }
  Node* input(size_t i) const;

  // Mutators (snake_case)
  void set_input(size_t i, Node* n);

  // Regular methods (PascalCase)
  void AddInput(Node* n);
  std::string ToString() const;

 private:
  NodeID id_;           // snake_case with trailing _
  Opcode opcode_;
  std::vector<Node*> inputs_;
};

enum class Opcode {
  kStart,               // kPascalCase
  kReturn,
  kAddI
};

std::string OpcodeToString(Opcode op);  // Regular function: PascalCase

}  // namespace sun
```

## When in Doubt

1. Consult the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
2. Look at similar existing code in this project
3. Prioritize readability and consistency
4. Ask for clarification in code reviews
