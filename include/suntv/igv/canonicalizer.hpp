#pragma once

namespace sun {
class Graph;

class Canonicalizer {
 public:
  Graph* canonicalize(Graph* raw);
};

}  // namespace sun
