#include "db/Database.hpp"

namespace libraflow::db {

Database::Database(const std::string& path) {
  const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
  const int code = sqlite3_open_v2(path.c_str(), &db_, flags, nullptr);
  if (code != SQLITE_OK) {
    std::string message = db_ ? sqlite3_errmsg(db_) : "unknown sqlite error";
    if (db_) {
      sqlite3_close(db_);
      db_ = nullptr;
    }
    throw std::runtime_error("Failed to open database: " + message);
  }
  execute("PRAGMA foreign_keys = ON;");
}

Database::~Database() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
  }
}

void Database::execute(const std::string& sql) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  char* error = nullptr;
  const int code = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
  if (code != SQLITE_OK) {
    const std::string message = error ? error : "sqlite exec error";
    sqlite3_free(error);
    throw std::runtime_error(message);
  }
}

void Database::transaction(const std::function<void(sqlite3*)>& operation) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  throw_on_error(sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, nullptr), db_, "begin transaction");
  try {
    operation(db_);
    throw_on_error(sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr), db_, "commit transaction");
  } catch (...) {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    throw;
  }
}

void Database::with_connection(const std::function<void(sqlite3*)>& operation) {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  operation(db_);
}

sqlite3_int64 Database::last_insert_rowid() const {
  std::lock_guard<std::recursive_mutex> lock(mutex_);
  return sqlite3_last_insert_rowid(db_);
}

Statement::Statement(sqlite3* db, const std::string& sql) {
  throw_on_error(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr), db, "prepare statement");
}

Statement::~Statement() {
  if (stmt_ != nullptr) {
    sqlite3_finalize(stmt_);
  }
}

void throw_on_error(int code, sqlite3* db, const std::string& context) {
  if (code != SQLITE_OK && code != SQLITE_DONE && code != SQLITE_ROW) {
    throw std::runtime_error(context + ": " + sqlite3_errmsg(db));
  }
}

}  // namespace libraflow::db
