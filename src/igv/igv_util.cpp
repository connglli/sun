#include "suntv/igv/igv_util.hpp"

#include <fstream>
#include <pugixml.hpp>

#include "suntv/util/logging.hpp"

namespace sun {

std::vector<GraphInfo> IGVUtil::ListGraphs(const std::string& path) {
  std::vector<GraphInfo> result;

  pugi::xml_document doc;
  pugi::xml_parse_result parse_result = doc.load_file(path.c_str());

  if (!parse_result) {
    Logger::Error("Failed to parse XML file: " + path);
    Logger::Error(parse_result.description());
    return result;
  }

  // Navigate to graphs: graphDocument -> group -> graph(s)
  pugi::xml_node graph_doc = doc.child("graphDocument");
  if (!graph_doc) {
    Logger::Error("No graphDocument element found");
    return result;
  }

  pugi::xml_node group = graph_doc.child("group");
  if (!group) {
    Logger::Error("No group element found");
    return result;
  }

  // Iterate through all graph elements
  size_t index = 0;
  for (pugi::xml_node graph : group.children("graph")) {
    GraphInfo info;
    info.index = index++;

    // Get graph name from attribute or properties
    const char* name_attr = graph.attribute("name").value();
    if (name_attr && name_attr[0] != '\0') {
      info.name = name_attr;
    } else {
      // Try to find name in properties
      pugi::xml_node props = graph.child("properties");
      if (props) {
        for (pugi::xml_node p : props.children("p")) {
          if (std::string(p.attribute("name").value()) == "name") {
            info.name = p.child_value();
            break;
          }
        }
      }
    }

    // Count nodes
    pugi::xml_node nodes = graph.child("nodes");
    info.num_nodes = 0;
    if (nodes) {
      for (pugi::xml_node node : nodes.children("node")) {
        (void)node;  // Suppress unused variable warning
        info.num_nodes++;
      }
    }

    // Count edges
    pugi::xml_node edges = graph.child("edges");
    info.num_edges = 0;
    if (edges) {
      for (pugi::xml_node edge : edges.children("edge")) {
        (void)edge;  // Suppress unused variable warning
        info.num_edges++;
      }
    }

    result.push_back(info);
  }

  return result;
}

bool IGVUtil::ExtractGraph(const std::string& input_path, size_t graph_index,
                           const std::string& output_path) {
  pugi::xml_document doc;
  pugi::xml_parse_result parse_result = doc.load_file(input_path.c_str());

  if (!parse_result) {
    Logger::Error("Failed to parse XML file: " + input_path);
    Logger::Error(parse_result.description());
    return false;
  }

  // Navigate to graphs
  pugi::xml_node graph_doc = doc.child("graphDocument");
  if (!graph_doc) {
    Logger::Error("No graphDocument element found");
    return false;
  }

  pugi::xml_node group = graph_doc.child("group");
  if (!group) {
    Logger::Error("No group element found");
    return false;
  }

  // Find the graph at the specified index
  size_t current_index = 0;
  pugi::xml_node target_graph;
  for (pugi::xml_node graph : group.children("graph")) {
    if (current_index == graph_index) {
      target_graph = graph;
      break;
    }
    current_index++;
  }

  if (!target_graph) {
    Logger::Error("Graph at index " + std::to_string(graph_index) +
                  " not found");
    return false;
  }

  // Create a new document with just this graph
  pugi::xml_document output_doc;

  // Copy graphDocument
  pugi::xml_node output_graph_doc = output_doc.append_child("graphDocument");

  // Copy group (with method info if it exists)
  pugi::xml_node output_group = output_graph_doc.append_child("group");

  // Copy group properties if they exist
  pugi::xml_node group_props = group.child("properties");
  if (group_props) {
    output_group.append_copy(group_props);
  }

  // Copy method if it exists
  pugi::xml_node method = group.child("method");
  if (method) {
    output_group.append_copy(method);
  }

  // Copy the target graph
  output_group.append_copy(target_graph);

  // Save to file
  if (!output_doc.save_file(output_path.c_str())) {
    Logger::Error("Failed to save extracted graph to: " + output_path);
    return false;
  }

  Logger::Info("Extracted graph " + std::to_string(graph_index) + " to " +
               output_path);
  return true;
}

bool IGVUtil::ExtractGraph(const std::string& input_path,
                           const std::string& graph_name,
                           const std::string& output_path) {
  pugi::xml_document doc;
  pugi::xml_parse_result parse_result = doc.load_file(input_path.c_str());

  if (!parse_result) {
    Logger::Error("Failed to parse XML file: " + input_path);
    Logger::Error(parse_result.description());
    return false;
  }

  // Navigate to graphs
  pugi::xml_node graph_doc = doc.child("graphDocument");
  if (!graph_doc) {
    Logger::Error("No graphDocument element found");
    return false;
  }

  pugi::xml_node group = graph_doc.child("group");
  if (!group) {
    Logger::Error("No group element found");
    return false;
  }

  // Find the graph with the specified name
  pugi::xml_node target_graph;
  for (pugi::xml_node graph : group.children("graph")) {
    const char* name_attr = graph.attribute("name").value();
    if (name_attr && graph_name == name_attr) {
      target_graph = graph;
      break;
    }

    // Also check properties for name
    pugi::xml_node props = graph.child("properties");
    if (props) {
      for (pugi::xml_node p : props.children("p")) {
        if (std::string(p.attribute("name").value()) == "name" &&
            graph_name == p.child_value()) {
          target_graph = graph;
          break;
        }
      }
    }

    if (target_graph) break;
  }

  if (!target_graph) {
    Logger::Error("Graph with name '" + graph_name + "' not found");
    return false;
  }

  // Create a new document with just this graph
  pugi::xml_document output_doc;

  // Copy graphDocument
  pugi::xml_node output_graph_doc = output_doc.append_child("graphDocument");

  // Copy group (with method info if it exists)
  pugi::xml_node output_group = output_graph_doc.append_child("group");

  // Copy group properties if they exist
  pugi::xml_node group_props = group.child("properties");
  if (group_props) {
    output_group.append_copy(group_props);
  }

  // Copy method if it exists
  pugi::xml_node method = group.child("method");
  if (method) {
    output_group.append_copy(method);
  }

  // Copy the target graph
  output_group.append_copy(target_graph);

  // Save to file
  if (!output_doc.save_file(output_path.c_str())) {
    Logger::Error("Failed to save extracted graph to: " + output_path);
    return false;
  }

  Logger::Info("Extracted graph '" + graph_name + "' to " + output_path);
  return true;
}

}  // namespace sun
