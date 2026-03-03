#pragma once

#include <string>

namespace libraflow::utils {

struct Config {
  std::string db_path{"libraflow.db"};
  double fine_per_day{1.5};
  int scanner_interval_seconds{20};
};

class ConfigLoader {
 public:
  static Config load(const std::string& path);
};

}  // namespace libraflow::utils
