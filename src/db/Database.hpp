#pragma once

#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>

#include <sqlite3.h>

namespace libraflow::db {

class Database {
 public:
  explicit Database(const std::string& path);
  ~Database();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void execute(const std::string& sql);
  void transaction(const std::function<void(sqlite3*)>& operation);
  void with_connection(const std::function<void(sqlite3*)>& operation);
  [[nodiscard]] sqlite3_int64 last_insert_rowid() const;

 private:
  sqlite3* db_{nullptr};
  mutable std::recursive_mutex mutex_;
};

class Statement {
 public:
  Statement(sqlite3* db, const std::string& sql);
  ~Statement();

  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  sqlite3_stmt* get() { return stmt_; }

 private:
  sqlite3_stmt* stmt_{nullptr};
};

void throw_on_error(int code, sqlite3* db, const std::string& context);

}  // namespace libraflow::db
