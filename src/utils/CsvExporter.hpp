#pragma once

#include <string>
#include <vector>

namespace libraflow::utils {

class CsvExporter {
 public:
  static void export_rows(const std::string& path,
                          const std::vector<std::string>& headers,
                          const std::vector<std::vector<std::string>>& rows);
};

}  // namespace libraflow::utils
