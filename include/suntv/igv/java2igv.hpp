#pragma once

#include <string>

namespace sun {

/**
 * Java to IGV utility for dumping IGV XML from Java source files.
 */
class Java2IGV {
 public:
  /**
   * Dump IGV XML from a Java source file.
   * This function compiles the Java file and runs it with JVM flags to
   * generate an IGV XML dump.
   *
   * @param java_file Path to Java source file
   * @param output_file Path to output IGV XML file
   * @param method_name Method to compile (default: "compute")
   * @return true on success, false on failure
   */
  static bool DumpIGV(const std::string& java_file,
                      const std::string& output_file,
                      const std::string& method_name = "compute");

  /**
   * Get the path to the java binary.
   * Checks JAVA_BIN environment variable, falls back to "java".
   */
  static std::string GetJavaBin();

  /**
   * Get the path to the javac binary.
   * Checks JAVAC_BIN environment variable, falls back to "javac".
   */
  static std::string GetJavacBin();
};

}  // namespace sun
