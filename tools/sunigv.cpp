#include <filesystem>
#include <iostream>
#include <string>

#include "suntv/igv/igv_util.hpp"
#include "suntv/igv/java2igv.hpp"
#include "suntv/util/cxxopts.hpp"

using namespace sun;
namespace fs = std::filesystem;

int DumpCommand(int argc, char** argv) {
  cxxopts::Options options("sunigv dump", "Dump IGV from Java source");
  // clang-format off
  options.add_options()
    ("java-file", "Java source file", cxxopts::value<std::string>())
    ("o,output", "Output IGV XML file", cxxopts::value<std::string>())
    ("m,method", "Method name to compile (default: compute)", cxxopts::value<std::string>()->default_value("compute"))
    ("h,help", "Print help");
  options.parse_positional({"java-file"});
  options.positional_help("<java-file>");
  // clang-format on

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << "\n";
      return 0;
    }

    if (!result.count("java-file")) {
      std::cerr << "Error: Java file not specified\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    std::string java_file = result["java-file"].as<std::string>();
    std::string method = result["method"].as<std::string>();

    // Determine output file
    std::string output_file;
    if (result.count("output")) {
      output_file = result["output"].as<std::string>();
    } else {
      // Default: same name as Java file but with .xml extension
      fs::path java_path(java_file);
      output_file = java_path.stem().string() + ".xml";
    }

    // Call Java2IGV
    bool success = Java2IGV::DumpIGV(java_file, output_file, method);
    return success ? 0 : 1;

  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    std::cout << options.help() << "\n";
    return 1;
  }
}

int ListCommand(int argc, char** argv) {
  cxxopts::Options options("sunigv list", "List graphs in IGV file");
  // clang-format off
  options.add_options()
    ("igv-file", "IGV XML file", cxxopts::value<std::string>())("h,help", "Print help");
  options.parse_positional({"igv-file"});
  options.positional_help("<igv-file>");
  // clang-format on

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << "\n";
      return 0;
    }

    if (!result.count("igv-file")) {
      std::cerr << "Error: IGV file not specified\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    std::string igv_file = result["igv-file"].as<std::string>();

    // List graphs
    auto graphs = IGVUtil::ListGraphs(igv_file);

    if (graphs.empty()) {
      std::cerr << "No graphs found or failed to parse file\n";
      return 1;
    }

    // Print header
    std::cout << "Graphs in " << igv_file << ":\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << "Index  Nodes  Edges  Name\n";
    std::cout << std::string(80, '-') << "\n";

    // Print each graph
    for (const auto& graph : graphs) {
      printf("%-6zu %-6zu %-6zu %s\n", graph.index, graph.num_nodes,
             graph.num_edges, graph.name.c_str());
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "Total: " << graphs.size() << " graph(s)\n";

    return 0;

  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    std::cout << options.help() << "\n";
    return 1;
  }
}

int ExtractCommand(int argc, char** argv) {
  cxxopts::Options options("sunigv extract", "Extract graph from IGV file");
  // clang-format off
  options.add_options()
    ("igv-file", "IGV XML file", cxxopts::value<std::string>())
    ("i,index", "Graph index to extract", cxxopts::value<size_t>())
    ("n,name", "Graph name to extract", cxxopts::value<std::string>())
    ("o,output", "Output IGV XML file", cxxopts::value<std::string>())
    ("h,help", "Print help");
  options.parse_positional({"igv-file"});
  options.positional_help("<igv-file>");
  // clang-format on

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << "\n";
      return 0;
    }

    if (!result.count("igv-file")) {
      std::cerr << "Error: IGV file not specified\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    if (!result.count("output")) {
      std::cerr << "Error: Output file not specified (-o)\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    if (!result.count("index") && !result.count("name")) {
      std::cerr << "Error: Must specify either --index or --name\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    if (result.count("index") && result.count("name")) {
      std::cerr << "Error: Cannot specify both --index and --name\n\n";
      std::cout << options.help() << "\n";
      return 1;
    }

    std::string igv_file = result["igv-file"].as<std::string>();
    std::string output_file = result["output"].as<std::string>();

    bool success = false;
    if (result.count("index")) {
      size_t index = result["index"].as<size_t>();
      success = IGVUtil::ExtractGraph(igv_file, index, output_file);
    } else {
      std::string name = result["name"].as<std::string>();
      success = IGVUtil::ExtractGraph(igv_file, name, output_file);
    }

    return success ? 0 : 1;

  } catch (const cxxopts::exceptions::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    std::cout << options.help() << "\n";
    return 1;
  }
}

int main(int argc, char** argv) {
  // Main options for sunigv
  cxxopts::Options options("sunigv", "IGV utility tool");

  // clang-format off
  options.add_options()
    ("command", "Command to execute (dump, list, extract)", cxxopts::value<std::string>())("h,help", "Print help");
  options.parse_positional({"command"});
  options.positional_help("<command>");
  // clang-format on

  // Add command descriptions
  options.custom_help("[OPTION...] <command> [<args>...]");
  std::string help_epilog =
      "\nCommands:\n"
      "  dump       Compile Java source and generate IGV XML dump\n"
      "  list       List all graphs in an IGV XML file\n"
      "  extract    Extract a specific graph to a separate IGV XML file\n"
      "\n"
      "Examples:\n"
      "  sunigv dump Fibonacci.java -o fibonacci.xml -m compute\n"
      "  sunigv list fibonacci.xml\n"
      "  sunigv extract fibonacci.xml -i 0 -o after_parsing.xml\n"
      "  sunigv extract fibonacci.xml -n \"After Parsing\" -o "
      "after_parsing.xml\n"
      "\n"
      "Use 'sunigv <command> --help' for more information on a command.\n";

  if (argc < 2) {
    std::cout << options.help() << help_epilog;
    return 1;
  }

  std::string command = argv[1];

  if (command == "dump") {
    // Shift arguments: sunigv dump <args> -> sunigv <args>
    return DumpCommand(argc - 1, argv + 1);
  } else if (command == "list") {
    return ListCommand(argc - 1, argv + 1);
  } else if (command == "extract") {
    return ExtractCommand(argc - 1, argv + 1);
  } else if (command == "-h" || command == "--help") {
    std::cout << options.help() << help_epilog;
    return 0;
  } else {
    std::cerr << "Error: Unknown command '" << command << "'\n\n";
    std::cout << options.help() << help_epilog;
    return 1;
  }
}
