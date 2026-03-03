#include "utils/Time.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace libraflow::utils {

namespace {

std::tm safe_localtime(std::time_t time) {
  std::tm result{};
#ifdef _WIN32
  localtime_s(&result, &time);
#else
  localtime_r(&time, &result);
#endif
  return result;
}

}  // namespace

std::string format_time_point(std::chrono::system_clock::time_point point) {
  const std::time_t tt = std::chrono::system_clock::to_time_t(point);
  const std::tm tm = safe_localtime(tt);
  std::ostringstream output;
  output << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return output.str();
}

std::string now_iso() {
  return format_time_point(std::chrono::system_clock::now());
}

std::string add_days_iso(int days) {
  return format_time_point(std::chrono::system_clock::now() + std::chrono::hours(24 * days));
}

std::chrono::system_clock::time_point parse_iso(const std::string& iso) {
  std::tm tm{};
  std::istringstream input(iso);
  input >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
  if (input.fail()) {
    return std::chrono::system_clock::now();
  }
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

long long days_overdue(const std::string& due_at_iso, const std::string& reference_iso) {
  const auto due = parse_iso(due_at_iso);
  const auto reference = parse_iso(reference_iso);
  if (reference <= due) {
    return 0;
  }
  const auto diff = std::chrono::duration_cast<std::chrono::hours>(reference - due).count();
  return (diff + 23) / 24;
}

}  // namespace libraflow::utils
