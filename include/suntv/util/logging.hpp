#pragma once

#include <iostream>
#include <string>

namespace sun {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

class Logger {
 public:
  static void set_level(LogLevel level);
  static LogLevel get_level();

  static void trace(const std::string& msg);
  static void debug(const std::string& msg);
  static void info(const std::string& msg);
  static void warn(const std::string& msg);
  static void error(const std::string& msg);

 private:
  static LogLevel level_;
  static void log(LogLevel level, const std::string& prefix,
                  const std::string& msg);
};

}  // namespace sun
