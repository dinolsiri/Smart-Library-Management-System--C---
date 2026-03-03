#include "utils/Config.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace libraflow::utils {

Config ConfigLoader::load(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Unable to open config file: " + path);
  }

  nlohmann::json json;
  input >> json;

  Config config;
  config.db_path = json.value("db_path", config.db_path);
  config.fine_per_day = json.value("fine_per_day", config.fine_per_day);
  config.scanner_interval_seconds = json.value("scanner_interval_seconds", config.scanner_interval_seconds);
  return config;
}

}  // namespace libraflow::utils
