#include <iostream>
#include <stdexcept>

#include "utils/Hashing.hpp"
#include "utils/Time.hpp"

namespace {

void expect(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main() {
  try {
    const std::string password = "secret123";
    const auto digest = libraflow::utils::PasswordHasher::hash_password(password);
    expect(libraflow::utils::PasswordHasher::verify_password(password, digest), "Password verification failed");
    expect(!libraflow::utils::PasswordHasher::verify_password("wrong", digest), "Password verification accepted wrong password");
    expect(libraflow::utils::days_overdue("2024-01-01 00:00:00", "2024-01-03 09:00:00") == 3, "days_overdue mismatch");
    std::cout << "All LibraFlow tests passed.\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Test failure: " << ex.what() << '\n';
    return 1;
  }
}
