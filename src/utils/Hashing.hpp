#pragma once

#include <string>

namespace libraflow::utils {

class PasswordHasher {
 public:
  static std::string hash_password(const std::string& password);
  static bool verify_password(const std::string& password, const std::string& stored);

 private:
  static std::string random_salt();
  static std::string sha256_hex(const std::string& input);
};

}  // namespace libraflow::utils
