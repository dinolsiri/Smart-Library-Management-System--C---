#include "utils/Logger.hpp"

#include <fstream>

#include "utils/Time.hpp"

namespace libraflow::utils {

Logger::Logger(std::string path) : path_(std::move(path)) {}

void Logger::info(const std::string& message) {
  write("INFO", message);
}

void Logger::error(const std::string& message) {
  write("ERROR", message);
}

void Logger::write(const std::string& level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::ofstream output(path_, std::ios::app);
  output << "[" << now_iso() << "] [" << level << "] " << message << '\n';
}

}  // namespace libraflow::utils
