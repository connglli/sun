#include "suntv/igv/parser.hpp"

#include <pugixml.hpp>

#include "suntv/igv/canonicalizer.hpp"
#include "suntv/ir/graph.hpp"
#include "suntv/ir/node.hpp"
#include "suntv/ir/opcode.hpp"
#include "suntv/util/logging.hpp"

namespace sun {

// Implementation class (PIMPL pattern)
class IGVParser::Impl {
 public:
  std::unique_ptr<Graph> parse(const std::string& path) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(path.c_str());

    if (!result) {
      Logger::error("Failed to parse XML file: " + path);
      Logger::error(result.description());
      return nullptr;
    }

    // Navigate to first graph
    pugi::xml_node graph_node =
        doc.child("graphDocument").child("group").child("graph");

    if (!graph_node) {
      Logger::error("No graph found in IGV file");
      return nullptr;
    }

    return parse_graph(graph_node);
  }

 private:
  std::unique_ptr<Graph> parse_graph(pugi::xml_node graph_node) {
    auto graph = std::make_unique<Graph>();

    // Parse nodes
    pugi::xml_node nodes = graph_node.child("nodes");
    for (pugi::xml_node node : nodes.children("node")) {
      parse_node(node, graph.get());
    }

    // Parse edges
    pugi::xml_node edges = graph_node.child("edges");
    for (pugi::xml_node edge : edges.children("edge")) {
      parse_edge(edge, graph.get());
    }

    // Canonicalize and validate the graph
    Canonicalizer canon;
    Graph* validated = canon.canonicalize(graph.get());
    if (!validated) {
      Logger::error("Graph failed canonicalization/validation");
      return nullptr;
    }

    return graph;
  }

  void parse_node(pugi::xml_node node, Graph* graph) {
    // Get node ID
    const char* id_str = node.attribute("id").value();
    if (!id_str || id_str[0] == '\0') {
      Logger::warn("Node missing ID, skipping");
      return;
    }

    NodeID id = std::atoi(id_str);

    // Get opcode name from properties
    pugi::xml_node props = node.child("properties");
    pugi::xml_node name_prop;
    for (pugi::xml_node p : props.children("p")) {
      if (std::string(p.attribute("name").value()) == "name") {
        name_prop = p;
        break;
      }
    }

    if (!name_prop) {
      Logger::warn("Node missing 'name' property, skipping");
      return;
    }

    std::string opcode_str = name_prop.child_value();
    Opcode opcode = string_to_opcode(opcode_str);

    if (opcode == Opcode::Unknown) {
      Logger::warn("Unknown opcode: " + opcode_str + ", skipping");
      return;
    }

    // Create node
    Node* n = graph->add_node(id, opcode);

    // Parse remaining properties
    for (pugi::xml_node p : props.children("p")) {
      std::string prop_name = p.attribute("name").value();
      if (prop_name == "name") continue;  // Already handled

      std::string prop_value = p.child_value();

      // Try to parse as int32
      char* end;
      long val = std::strtol(prop_value.c_str(), &end, 10);
      if (end != prop_value.c_str() && *end == '\0') {
        n->set_prop(prop_name, static_cast<int32_t>(val));
      } else {
        // Store as string
        n->set_prop(prop_name, prop_value);
      }
    }

    Logger::debug("Parsed node " + std::to_string(id) + ": " +
                  opcode_to_string(opcode));
  }

  void parse_edge(pugi::xml_node edge, Graph* graph) {
    // Get from/to node IDs
    const char* from_str = edge.attribute("from").value();
    const char* to_str = edge.attribute("to").value();

    if (!from_str || !to_str) {
      Logger::warn("Edge missing from/to attributes, skipping");
      return;
    }

    NodeID from_id = std::atoi(from_str);
    NodeID to_id = std::atoi(to_str);

    Node* from_node = graph->node(from_id);
    Node* to_node = graph->node(to_id);

    if (!from_node || !to_node) {
      Logger::warn("Edge refers to non-existent node, skipping");
      return;
    }

    // Get input index (toIndex or index attribute)
    const char* to_index_str = edge.attribute("toIndex").value();
    if (!to_index_str || to_index_str[0] == '\0') {
      to_index_str = edge.attribute("index").value();
    }

    size_t to_index = 0;
    if (to_index_str && to_index_str[0] != '\0') {
      to_index = std::atoi(to_index_str);
    }

    // Add edge: to_node->set_input(to_index, from_node)
    to_node->set_input(to_index, from_node);

    Logger::debug("Parsed edge: " + std::to_string(from_id) + " -> " +
                  std::to_string(to_id) + "[" + std::to_string(to_index) + "]");
  }
};

// IGVParser implementation

IGVParser::IGVParser() : impl_(std::make_unique<Impl>()) {}

IGVParser::~IGVParser() = default;

std::unique_ptr<Graph> IGVParser::parse(const std::string& path) {
  return impl_->parse(path);
}

}  // namespace sun
