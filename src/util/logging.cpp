#include "suntv/util/logging.hpp"

namespace sun {

LogLevel Logger::level_ = LogLevel::INFO;

void Logger::set_level(LogLevel level) { level_ = level; }

LogLevel Logger::get_level() { return level_; }

void Logger::log(LogLevel level, const std::string& prefix,
                 const std::string& msg) {
  if (level < level_) {
    return;
  }
  std::cerr << prefix << msg << std::endl;
}

void Logger::trace(const std::string& msg) {
  log(LogLevel::TRACE, "[TRACE] ", msg);
}

void Logger::debug(const std::string& msg) {
  log(LogLevel::DEBUG, "[DEBUG] ", msg);
}

void Logger::info(const std::string& msg) {
  log(LogLevel::INFO, "[INFO] ", msg);
}

void Logger::warn(const std::string& msg) {
  log(LogLevel::WARN, "[WARN] ", msg);
}

void Logger::error(const std::string& msg) {
  log(LogLevel::ERROR, "[ERROR] ", msg);
}

}  // namespace sun
