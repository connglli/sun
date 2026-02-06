#include "suntv/igv/java2igv.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "suntv/util/logging.hpp"

namespace sun {
namespace fs = std::filesystem;

std::string Java2IGV::GetJavaBin() {
  const char* env = std::getenv("JAVA_BIN");
  return env ? std::string(env) : "java";
}

std::string Java2IGV::GetJavacBin() {
  const char* env = std::getenv("JAVAC_BIN");
  return env ? std::string(env) : "javac";
}

bool Java2IGV::DumpIGV(const std::string& java_file,
                       const std::string& output_file,
                       const std::string& method_name) {
  // Validate input
  if (!fs::exists(java_file)) {
    Logger::Error("Java file not found: " + java_file);
    return false;
  }

  fs::path java_path(java_file);
  std::string class_name = java_path.stem().string();
  fs::path java_dir = java_path.parent_path();
  if (java_dir.empty()) {
    java_dir = ".";
  }

  Logger::Info("=== Java to IGV Compiler ===");
  Logger::Info("Source:  " + java_file);
  Logger::Info("Class:   " + class_name);
  Logger::Info("Method:  " + method_name);
  Logger::Info("Output:  " + output_file);

  // Step 1: Compile Java source
  Logger::Info("[1/3] Compiling Java source...");

  std::string javac_bin = GetJavacBin();
  std::string compile_cmd = javac_bin + " \"" + java_file + "\" 2>&1";

  FILE* compile_pipe = popen(compile_cmd.c_str(), "r");
  if (!compile_pipe) {
    Logger::Error("Failed to execute javac");
    return false;
  }

  std::string compile_output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), compile_pipe) != nullptr) {
    compile_output += buffer;
  }

  int compile_status = pclose(compile_pipe);
  if (compile_status != 0) {
    Logger::Error("Compilation failed:");
    Logger::Error(compile_output);
    return false;
  }

  Logger::Info("✓ Compiled successfully");

  // Step 2: Test JVM capabilities
  Logger::Info("[2/3] Testing JVM capabilities...");

  std::string java_bin = GetJavaBin();
  std::string test_cmd = java_bin +
                         " -XX:+UnlockDiagnosticVMOptions -XX:+PrintIdeal "
                         "-version 2>&1";

  FILE* test_pipe = popen(test_cmd.c_str(), "r");
  if (!test_pipe) {
    Logger::Error("Failed to execute java");
    return false;
  }

  std::string test_output;
  while (fgets(buffer, sizeof(buffer), test_pipe) != nullptr) {
    test_output += buffer;
  }
  pclose(test_pipe);

  // Check if we have debug JVM support
  if (test_output.find("notproduct") != std::string::npos) {
    Logger::Warn("Standard JVM detected (no PrintIdeal support)");
    Logger::Warn("You need a debug/fastdebug JDK build for IGV XML generation");
    Logger::Warn("Attempting to generate IGV anyway...");
  }

  // Step 3: Generate IGV XML
  Logger::Info("[3/3] Generating IGV XML dump...");

  std::string temp_igv_file = class_name + "_igv.xml";

  std::ostringstream cmd;
  cmd << "cd \"" << java_dir.string() << "\" && " << java_bin << " -Xcomp"
      << " -XX:+UnlockDiagnosticVMOptions" << " -XX:+PrintIdeal"
      << " -XX:PrintIdealGraphLevel=2" << " -XX:PrintIdealGraphFile=\""
      << temp_igv_file << "\"" << " -XX:CompileCommand=compileonly,"
      << class_name << "::" << method_name << " -XX:-TieredCompilation"
      << " -XX:-UseOnStackReplacement" << " -XX:-BackgroundCompilation"
      << " -XX:+PrintCompilation" << " " << class_name << " 2>&1";

  std::string igv_cmd = cmd.str();

  FILE* igv_pipe = popen(igv_cmd.c_str(), "r");
  if (!igv_pipe) {
    Logger::Error("Failed to execute java for IGV generation");
    return false;
  }

  std::string igv_output;
  while (fgets(buffer, sizeof(buffer), igv_pipe) != nullptr) {
    igv_output += buffer;
  }

  int igv_status = pclose(igv_pipe);
  (void)igv_status;  // Suppress unused variable warning

  // Check if IGV file was created
  fs::path temp_igv_path = java_dir / temp_igv_file;
  if (!fs::exists(temp_igv_path)) {
    Logger::Error("IGV XML not generated (method might not have compiled)");
    Logger::Error("JVM output:");
    Logger::Error(igv_output);
    Logger::Error("Try increasing warmup iterations in your Java program");
    return false;
  }

  Logger::Info("✓ IGV XML generated: " + temp_igv_path.string());

  // Move to output location
  fs::path output_path(output_file);
  fs::path output_dir = output_path.parent_path();
  if (!output_dir.empty() && !fs::exists(output_dir)) {
    fs::create_directories(output_dir);
  }

  try {
    fs::copy_file(temp_igv_path, output_path,
                  fs::copy_options::overwrite_existing);
    Logger::Info("=== SUCCESS ===");
    Logger::Info("IGV graph: " + output_file);
    return true;
  } catch (const std::exception& e) {
    Logger::Error("Failed to copy IGV file to output location: " +
                  std::string(e.what()));
    return false;
  }
}

}  // namespace sun
