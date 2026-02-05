#pragma once
#include <string>

namespace sun {
class Graph;

class IGVParser {
 public:
  Graph* parse(const std::string& path);
};

}  // namespace sun
