#include "db/Repositories.hpp"

#include "utils/Time.hpp"

namespace libraflow::db {

namespace {

using namespace libraflow::domain;

void bind_text(sqlite3_stmt* stmt, int index, const std::string& value) {
  sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

User read_user(sqlite3_stmt* stmt) {
  return User{
      sqlite3_column_int(stmt, 0),
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)),
      role_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))),
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)),
      sqlite3_column_int(stmt, 5) != 0};
}

Book read_book(sqlite3_stmt* stmt) {
  Book book;
  book.id = sqlite3_column_int(stmt, 0);
  book.isbn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
  book.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
  book.author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  book.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
  book.year = sqlite3_column_int(stmt, 5);
  book.total_copies = sqlite3_column_int(stmt, 6);
  book.available_copies = sqlite3_column_int(stmt, 7);
  book.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
  return book;
}

Member read_member(sqlite3_stmt* stmt) {
  Member member;
  member.id = sqlite3_column_int(stmt, 0);
  member.member_no = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
  member.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
  member.email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  member.phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
  member.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
  return member;
}

Loan read_loan(sqlite3_stmt* stmt) {
  Loan loan;
  loan.id = sqlite3_column_int(stmt, 0);
  loan.book_id = sqlite3_column_int(stmt, 1);
  loan.member_id = sqlite3_column_int(stmt, 2);
  loan.borrowed_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  loan.due_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
  if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
    loan.returned_at = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
  }
  loan.fine_amount = sqlite3_column_double(stmt, 6);
  loan.status = loan_status_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
  loan.book_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
  loan.member_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
  return loan;
}

Reservation read_reservation(sqlite3_stmt* stmt) {
  Reservation reservation;
  reservation.id = sqlite3_column_int(stmt, 0);
  reservation.book_id = sqlite3_column_int(stmt, 1);
  reservation.member_id = sqlite3_column_int(stmt, 2);
  reservation.reserved_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
  reservation.status = reservation_status_from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
  reservation.queue_position = sqlite3_column_int(stmt, 5);
  reservation.book_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
  reservation.member_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
  return reservation;
}

}  // namespace

domain::User UserRepository::create(const domain::User& user) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO users(username, password_hash, role, created_at, must_change_password) VALUES(?,?,?,?,?);");
    bind_text(stmt.get(), 1, user.username);
    bind_text(stmt.get(), 2, user.password_hash);
    bind_text(stmt.get(), 3, to_string(user.role));
    bind_text(stmt.get(), 4, user.created_at);
    sqlite3_bind_int(stmt.get(), 5, user.must_change_password ? 1 : 0);
    throw_on_error(sqlite3_step(stmt.get()), db, "insert user");
  });
  auto created = user;
  created.id = static_cast<int>(database_.last_insert_rowid());
  return created;
}

std::optional<domain::User> UserRepository::find_by_username(const std::string& username) {
  std::optional<domain::User> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, username, password_hash, role, created_at, must_change_password FROM users WHERE username = ?;");
    bind_text(stmt.get(), 1, username);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_user(stmt.get());
  });
  return result;
}

std::optional<domain::User> UserRepository::find_by_id(int id) {
  std::optional<domain::User> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, username, password_hash, role, created_at, must_change_password FROM users WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_user(stmt.get());
  });
  return result;
}

std::vector<domain::User> UserRepository::list_all() {
  std::vector<domain::User> users;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, username, password_hash, role, created_at, must_change_password FROM users ORDER BY username;");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) users.push_back(read_user(stmt.get()));
  });
  return users;
}

void UserRepository::update_password(int id, const std::string& password_hash, bool must_change_password) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE users SET password_hash = ?, must_change_password = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, password_hash);
    sqlite3_bind_int(stmt.get(), 2, must_change_password ? 1 : 0);
    sqlite3_bind_int(stmt.get(), 3, id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update password");
  });
}

domain::Book BookRepository::create(const domain::Book& book) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO books(isbn, title, author, category, year, total_copies, available_copies, created_at) VALUES(?,?,?,?,?,?,?,?);");
    bind_text(stmt.get(), 1, book.isbn);
    bind_text(stmt.get(), 2, book.title);
    bind_text(stmt.get(), 3, book.author);
    bind_text(stmt.get(), 4, book.category);
    sqlite3_bind_int(stmt.get(), 5, book.year);
    sqlite3_bind_int(stmt.get(), 6, book.total_copies);
    sqlite3_bind_int(stmt.get(), 7, book.available_copies);
    bind_text(stmt.get(), 8, book.created_at);
    throw_on_error(sqlite3_step(stmt.get()), db, "insert book");
  });
  auto created = book;
  created.id = static_cast<int>(database_.last_insert_rowid());
  return created;
}

void BookRepository::update(const domain::Book& book) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE books SET isbn = ?, title = ?, author = ?, category = ?, year = ?, total_copies = ?, available_copies = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, book.isbn);
    bind_text(stmt.get(), 2, book.title);
    bind_text(stmt.get(), 3, book.author);
    bind_text(stmt.get(), 4, book.category);
    sqlite3_bind_int(stmt.get(), 5, book.year);
    sqlite3_bind_int(stmt.get(), 6, book.total_copies);
    sqlite3_bind_int(stmt.get(), 7, book.available_copies);
    sqlite3_bind_int(stmt.get(), 8, book.id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update book");
  });
}

void BookRepository::remove(int id) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "DELETE FROM books WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    throw_on_error(sqlite3_step(stmt.get()), db, "delete book");
  });
}

std::optional<domain::Book> BookRepository::find_by_id(int id) {
  std::optional<domain::Book> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, isbn, title, author, category, year, total_copies, available_copies, created_at FROM books WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_book(stmt.get());
  });
  return result;
}

std::vector<domain::Book> BookRepository::search(const std::string& query) {
  std::vector<domain::Book> books;
  const std::string pattern = "%" + query + "%";
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT id, isbn, title, author, category, year, total_copies, available_copies, created_at
      FROM books WHERE title LIKE ? OR author LIKE ? OR isbn LIKE ? OR category LIKE ? ORDER BY title;
    )sql");
    for (int i = 1; i <= 4; ++i) bind_text(stmt.get(), i, pattern);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) books.push_back(read_book(stmt.get()));
  });
  return books;
}

std::vector<domain::Book> BookRepository::list_all() {
  std::vector<domain::Book> books;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, isbn, title, author, category, year, total_copies, available_copies, created_at FROM books ORDER BY title;");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) books.push_back(read_book(stmt.get()));
  });
  return books;
}

void BookRepository::update_availability(int id, int available_copies) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE books SET available_copies = ? WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, available_copies);
    sqlite3_bind_int(stmt.get(), 2, id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update availability");
  });
}

int BookRepository::total_count() {
  int count = 0;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT COUNT(*) FROM books;");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) count = sqlite3_column_int(stmt.get(), 0);
  });
  return count;
}

domain::Member MemberRepository::create(const domain::Member& member) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO members(member_no, name, email, phone, created_at) VALUES(?,?,?,?,?);");
    bind_text(stmt.get(), 1, member.member_no);
    bind_text(stmt.get(), 2, member.name);
    bind_text(stmt.get(), 3, member.email);
    bind_text(stmt.get(), 4, member.phone);
    bind_text(stmt.get(), 5, member.created_at);
    throw_on_error(sqlite3_step(stmt.get()), db, "insert member");
  });
  auto created = member;
  created.id = static_cast<int>(database_.last_insert_rowid());
  return created;
}

void MemberRepository::update(const domain::Member& member) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE members SET member_no = ?, name = ?, email = ?, phone = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, member.member_no);
    bind_text(stmt.get(), 2, member.name);
    bind_text(stmt.get(), 3, member.email);
    bind_text(stmt.get(), 4, member.phone);
    sqlite3_bind_int(stmt.get(), 5, member.id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update member");
  });
}

void MemberRepository::remove(int id) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "DELETE FROM members WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    throw_on_error(sqlite3_step(stmt.get()), db, "delete member");
  });
}

std::optional<domain::Member> MemberRepository::find_by_id(int id) {
  std::optional<domain::Member> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, member_no, name, email, phone, created_at FROM members WHERE id = ?;");
    sqlite3_bind_int(stmt.get(), 1, id);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_member(stmt.get());
  });
  return result;
}

std::vector<domain::Member> MemberRepository::list_all() {
  std::vector<domain::Member> members;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT id, member_no, name, email, phone, created_at FROM members ORDER BY name;");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) members.push_back(read_member(stmt.get()));
  });
  return members;
}

int MemberRepository::total_count() {
  int count = 0;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT COUNT(*) FROM members;");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) count = sqlite3_column_int(stmt.get(), 0);
  });
  return count;
}

domain::Loan LoanRepository::create(const domain::Loan& loan) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO loans(book_id, member_id, borrowed_at, due_at, returned_at, fine_amount, status) VALUES(?,?,?,?,?,?,?);");
    sqlite3_bind_int(stmt.get(), 1, loan.book_id);
    sqlite3_bind_int(stmt.get(), 2, loan.member_id);
    bind_text(stmt.get(), 3, loan.borrowed_at);
    bind_text(stmt.get(), 4, loan.due_at);
    sqlite3_bind_null(stmt.get(), 5);
    sqlite3_bind_double(stmt.get(), 6, loan.fine_amount);
    bind_text(stmt.get(), 7, to_string(loan.status));
    throw_on_error(sqlite3_step(stmt.get()), db, "insert loan");
  });
  auto created = loan;
  created.id = static_cast<int>(database_.last_insert_rowid());
  return created;
}

void LoanRepository::update_return(int loan_id, const std::string& returned_at, double fine_amount, domain::LoanStatus status) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE loans SET returned_at = ?, fine_amount = ?, status = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, returned_at);
    sqlite3_bind_double(stmt.get(), 2, fine_amount);
    bind_text(stmt.get(), 3, to_string(status));
    sqlite3_bind_int(stmt.get(), 4, loan_id);
    throw_on_error(sqlite3_step(stmt.get()), db, "return loan");
  });
}

void LoanRepository::update_due_date(int loan_id, const std::string& due_at) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE loans SET due_at = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, due_at);
    sqlite3_bind_int(stmt.get(), 2, loan_id);
    throw_on_error(sqlite3_step(stmt.get()), db, "renew loan");
  });
}

void LoanRepository::update_fine_and_status(int loan_id, double fine_amount, domain::LoanStatus status) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE loans SET fine_amount = ?, status = ? WHERE id = ?;");
    sqlite3_bind_double(stmt.get(), 1, fine_amount);
    bind_text(stmt.get(), 2, to_string(status));
    sqlite3_bind_int(stmt.get(), 3, loan_id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update loan");
  });
}

std::optional<domain::Loan> LoanRepository::find_active_by_book_member(int book_id, int member_id) {
  std::optional<domain::Loan> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT l.id, l.book_id, l.member_id, l.borrowed_at, l.due_at, l.returned_at, l.fine_amount, l.status, b.title, m.name
      FROM loans l JOIN books b ON b.id = l.book_id JOIN members m ON m.id = l.member_id
      WHERE l.book_id = ? AND l.member_id = ? AND l.returned_at IS NULL ORDER BY l.id DESC LIMIT 1;
    )sql");
    sqlite3_bind_int(stmt.get(), 1, book_id);
    sqlite3_bind_int(stmt.get(), 2, member_id);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_loan(stmt.get());
  });
  return result;
}

std::optional<domain::Loan> LoanRepository::find_by_id(int loan_id) {
  std::optional<domain::Loan> result;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT l.id, l.book_id, l.member_id, l.borrowed_at, l.due_at, l.returned_at, l.fine_amount, l.status, b.title, m.name
      FROM loans l JOIN books b ON b.id = l.book_id JOIN members m ON m.id = l.member_id
      WHERE l.id = ?;
    )sql");
    sqlite3_bind_int(stmt.get(), 1, loan_id);
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) result = read_loan(stmt.get());
  });
  return result;
}

std::vector<domain::Loan> LoanRepository::list_all() {
  std::vector<domain::Loan> loans;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT l.id, l.book_id, l.member_id, l.borrowed_at, l.due_at, l.returned_at, l.fine_amount, l.status, b.title, m.name
      FROM loans l JOIN books b ON b.id = l.book_id JOIN members m ON m.id = l.member_id ORDER BY l.borrowed_at DESC;
    )sql");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) loans.push_back(read_loan(stmt.get()));
  });
  return loans;
}

std::vector<domain::Loan> LoanRepository::list_overdue() {
  std::vector<domain::Loan> loans;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT l.id, l.book_id, l.member_id, l.borrowed_at, l.due_at, l.returned_at, l.fine_amount, l.status, b.title, m.name
      FROM loans l JOIN books b ON b.id = l.book_id JOIN members m ON m.id = l.member_id
      WHERE l.returned_at IS NULL AND l.status = 'OVERDUE' ORDER BY l.due_at ASC;
    )sql");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) loans.push_back(read_loan(stmt.get()));
  });
  return loans;
}

int LoanRepository::active_count() {
  int count = 0;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT COUNT(*) FROM loans WHERE returned_at IS NULL;");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) count = sqlite3_column_int(stmt.get(), 0);
  });
  return count;
}

int LoanRepository::overdue_count() {
  int count = 0;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT COUNT(*) FROM loans WHERE returned_at IS NULL AND status = 'OVERDUE';");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) count = sqlite3_column_int(stmt.get(), 0);
  });
  return count;
}

std::vector<domain::MostBorrowedRow> LoanRepository::most_borrowed(int limit) {
  std::vector<domain::MostBorrowedRow> rows;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT b.title, COUNT(l.id) FROM loans l JOIN books b ON b.id = l.book_id
      GROUP BY b.id, b.title ORDER BY COUNT(l.id) DESC, b.title ASC LIMIT ?;
    )sql");
    sqlite3_bind_int(stmt.get(), 1, limit);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
      rows.push_back({reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0)), sqlite3_column_int(stmt.get(), 1)});
    }
  });
  return rows;
}

domain::Reservation ReservationRepository::create(const domain::Reservation& reservation) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO reservations(book_id, member_id, reserved_at, status, queue_position) VALUES(?,?,?,?,?);");
    sqlite3_bind_int(stmt.get(), 1, reservation.book_id);
    sqlite3_bind_int(stmt.get(), 2, reservation.member_id);
    bind_text(stmt.get(), 3, reservation.reserved_at);
    bind_text(stmt.get(), 4, to_string(reservation.status));
    sqlite3_bind_int(stmt.get(), 5, reservation.queue_position);
    throw_on_error(sqlite3_step(stmt.get()), db, "insert reservation");
  });
  auto created = reservation;
  created.id = static_cast<int>(database_.last_insert_rowid());
  return created;
}

std::vector<domain::Reservation> ReservationRepository::list_all() {
  std::vector<domain::Reservation> reservations;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT r.id, r.book_id, r.member_id, r.reserved_at, r.status, r.queue_position, b.title, m.name
      FROM reservations r JOIN books b ON b.id = r.book_id JOIN members m ON m.id = r.member_id
      ORDER BY r.book_id, r.queue_position;
    )sql");
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) reservations.push_back(read_reservation(stmt.get()));
  });
  return reservations;
}

std::vector<domain::Reservation> ReservationRepository::list_pending_for_book(int book_id) {
  std::vector<domain::Reservation> reservations;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, R"sql(
      SELECT r.id, r.book_id, r.member_id, r.reserved_at, r.status, r.queue_position, b.title, m.name
      FROM reservations r JOIN books b ON b.id = r.book_id JOIN members m ON m.id = r.member_id
      WHERE r.book_id = ? AND r.status = 'PENDING' ORDER BY r.queue_position ASC;
    )sql");
    sqlite3_bind_int(stmt.get(), 1, book_id);
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) reservations.push_back(read_reservation(stmt.get()));
  });
  return reservations;
}

void ReservationRepository::update_status(int reservation_id, domain::ReservationStatus status) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "UPDATE reservations SET status = ? WHERE id = ?;");
    bind_text(stmt.get(), 1, to_string(status));
    sqlite3_bind_int(stmt.get(), 2, reservation_id);
    throw_on_error(sqlite3_step(stmt.get()), db, "update reservation");
  });
}

int ReservationRepository::count_all() {
  int count = 0;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT COUNT(*) FROM reservations;");
    if (sqlite3_step(stmt.get()) == SQLITE_ROW) count = sqlite3_column_int(stmt.get(), 0);
  });
  return count;
}

bool ReservationRepository::has_active_reservation(int book_id, int member_id) {
  bool exists = false;
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "SELECT 1 FROM reservations WHERE book_id = ? AND member_id = ? AND status IN ('PENDING','READY') LIMIT 1;");
    sqlite3_bind_int(stmt.get(), 1, book_id);
    sqlite3_bind_int(stmt.get(), 2, member_id);
    exists = sqlite3_step(stmt.get()) == SQLITE_ROW;
  });
  return exists;
}

void AuditLogRepository::log(int actor_user_id, const std::string& action, const std::string& entity, int entity_id) {
  database_.with_connection([&](sqlite3* db) {
    Statement stmt(db, "INSERT INTO audit_logs(actor_user_id, action, entity, entity_id, at) VALUES(?,?,?,?,?);");
    sqlite3_bind_int(stmt.get(), 1, actor_user_id);
    bind_text(stmt.get(), 2, action);
    bind_text(stmt.get(), 3, entity);
    sqlite3_bind_int(stmt.get(), 4, entity_id);
    bind_text(stmt.get(), 5, utils::now_iso());
    throw_on_error(sqlite3_step(stmt.get()), db, "insert audit");
  });
}

domain::DashboardStats DashboardRepository::stats() {
  domain::DashboardStats stats;
  stats.total_books = BookRepository(database_).total_count();
  stats.total_members = MemberRepository(database_).total_count();
  stats.active_loans = LoanRepository(database_).active_count();
  stats.overdue_loans = LoanRepository(database_).overdue_count();
  stats.reservations = ReservationRepository(database_).count_all();
  return stats;
}

}  // namespace libraflow::db
