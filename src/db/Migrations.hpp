#pragma once

#include "db/Database.hpp"
#include "utils/Logger.hpp"

namespace libraflow::db {

class Migrator {
 public:
  static void migrate(Database& database, utils::Logger& logger);
};

}  // namespace libraflow::db
