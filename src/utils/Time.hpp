#pragma once

#include <chrono>
#include <string>

namespace libraflow::utils {

std::string now_iso();
std::string add_days_iso(int days);
std::chrono::system_clock::time_point parse_iso(const std::string& iso);
std::string format_time_point(std::chrono::system_clock::time_point point);
long long days_overdue(const std::string& due_at_iso, const std::string& reference_iso);

}  // namespace libraflow::utils
