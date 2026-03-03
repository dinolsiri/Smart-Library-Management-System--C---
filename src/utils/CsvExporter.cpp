#include "utils/CsvExporter.hpp"

#include <filesystem>
#include <fstream>

namespace libraflow::utils {

namespace {

std::string escape(const std::string& value) {
  std::string result = "\"";
  for (char ch : value) {
    if (ch == '"') {
      result += "\"\"";
    } else {
      result += ch;
    }
  }
  result += "\"";
  return result;
}

}  // namespace

void CsvExporter::export_rows(const std::string& path,
                              const std::vector<std::string>& headers,
                              const std::vector<std::vector<std::string>>& rows) {
  const auto file_path = std::filesystem::path(path);
  std::filesystem::create_directories(file_path.parent_path());

  std::ofstream output(path, std::ios::trunc);
  for (std::size_t i = 0; i < headers.size(); ++i) {
    output << escape(headers[i]) << (i + 1 < headers.size() ? "," : "\n");
  }
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) {
      output << escape(row[i]) << (i + 1 < row.size() ? "," : "\n");
    }
  }
}

}  // namespace libraflow::utils
