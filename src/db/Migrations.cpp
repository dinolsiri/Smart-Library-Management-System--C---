#include "db/Migrations.hpp"

#include "db/Repositories.hpp"
#include "utils/Hashing.hpp"
#include "utils/Time.hpp"

namespace libraflow::db {

void Migrator::migrate(Database& database, utils::Logger& logger) {
  database.execute(R"sql(
    CREATE TABLE IF NOT EXISTS users(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      username TEXT NOT NULL UNIQUE,
      password_hash TEXT NOT NULL,
      role TEXT NOT NULL,
      created_at TEXT NOT NULL,
      must_change_password INTEGER NOT NULL DEFAULT 0
    );

    CREATE TABLE IF NOT EXISTS books(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      isbn TEXT NOT NULL UNIQUE,
      title TEXT NOT NULL,
      author TEXT NOT NULL,
      category TEXT NOT NULL,
      year INTEGER NOT NULL,
      total_copies INTEGER NOT NULL,
      available_copies INTEGER NOT NULL,
      created_at TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS members(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      member_no TEXT NOT NULL UNIQUE,
      name TEXT NOT NULL,
      email TEXT NOT NULL,
      phone TEXT NOT NULL,
      created_at TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS loans(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      book_id INTEGER NOT NULL,
      member_id INTEGER NOT NULL,
      borrowed_at TEXT NOT NULL,
      due_at TEXT NOT NULL,
      returned_at TEXT NULL,
      fine_amount REAL NOT NULL DEFAULT 0,
      status TEXT NOT NULL,
      FOREIGN KEY(book_id) REFERENCES books(id),
      FOREIGN KEY(member_id) REFERENCES members(id)
    );

    CREATE TABLE IF NOT EXISTS reservations(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      book_id INTEGER NOT NULL,
      member_id INTEGER NOT NULL,
      reserved_at TEXT NOT NULL,
      status TEXT NOT NULL,
      queue_position INTEGER NOT NULL,
      FOREIGN KEY(book_id) REFERENCES books(id),
      FOREIGN KEY(member_id) REFERENCES members(id)
    );

    CREATE TABLE IF NOT EXISTS audit_logs(
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      actor_user_id INTEGER NOT NULL,
      action TEXT NOT NULL,
      entity TEXT NOT NULL,
      entity_id INTEGER NOT NULL,
      at TEXT NOT NULL,
      FOREIGN KEY(actor_user_id) REFERENCES users(id)
    );

    CREATE INDEX IF NOT EXISTS idx_books_title ON books(title);
    CREATE INDEX IF NOT EXISTS idx_books_author ON books(author);
    CREATE INDEX IF NOT EXISTS idx_books_isbn ON books(isbn);
    CREATE INDEX IF NOT EXISTS idx_books_category ON books(category);
    CREATE INDEX IF NOT EXISTS idx_loans_due_status ON loans(due_at, status);
    CREATE INDEX IF NOT EXISTS idx_loans_member ON loans(member_id, status);
    CREATE INDEX IF NOT EXISTS idx_reservations_book_queue ON reservations(book_id, status, queue_position);
  )sql");

  UserRepository users(database);
  if (!users.find_by_username("admin").has_value()) {
    domain::User admin{};
    admin.username = "admin";
    admin.password_hash = utils::PasswordHasher::hash_password("admin123");
    admin.role = domain::Role::Admin;
    admin.created_at = utils::now_iso();
    admin.must_change_password = true;
    const auto created = users.create(admin);
    AuditLogRepository(database).log(created.id, "SEED", "users", created.id);
    logger.info("Seeded default admin account.");
  }
}

}  // namespace libraflow::db
