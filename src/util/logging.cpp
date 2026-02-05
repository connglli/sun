#include "suntv/util/logging.hpp"

namespace sun {

LogLevel Logger::level_ = LogLevel::INFO;

void Logger::SetLevel(LogLevel level) { level_ = level; }

LogLevel Logger::GetLevel() { return level_; }

void Logger::Log(LogLevel level, const std::string& prefix,
                 const std::string& msg) {
  if (level < level_) {
    return;
  }
  std::cerr << prefix << msg << std::endl;
}

void Logger::Trace(const std::string& msg) {
  Log(LogLevel::TRACE, "[TRACE] ", msg);
}

void Logger::Debug(const std::string& msg) {
  Log(LogLevel::DEBUG, "[DEBUG] ", msg);
}

void Logger::Info(const std::string& msg) {
  Log(LogLevel::INFO, "[INFO] ", msg);
}

void Logger::Warn(const std::string& msg) {
  Log(LogLevel::WARN, "[WARN] ", msg);
}

void Logger::Error(const std::string& msg) {
  Log(LogLevel::ERROR, "[ERROR] ", msg);
}

}  // namespace sun
