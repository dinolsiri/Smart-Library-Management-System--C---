#pragma once

#include <mutex>
#include <string>

namespace libraflow::utils {

class Logger {
 public:
  explicit Logger(std::string path);
  void info(const std::string& message);
  void error(const std::string& message);

 private:
  void write(const std::string& level, const std::string& message);

  std::string path_;
  std::mutex mutex_;
};

}  // namespace libraflow::utils
