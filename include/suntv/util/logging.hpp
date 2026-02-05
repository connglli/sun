#pragma once

#include <iostream>
#include <string>

namespace sun {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };

class Logger {
 public:
  static void SetLevel(LogLevel level);
  static LogLevel GetLevel();

  static void Trace(const std::string& msg);
  static void Debug(const std::string& msg);
  static void Info(const std::string& msg);
  static void Warn(const std::string& msg);
  static void Error(const std::string& msg);

 private:
  static LogLevel level_;
  static void Log(LogLevel level, const std::string& prefix,
                  const std::string& msg);
};

}  // namespace sun
