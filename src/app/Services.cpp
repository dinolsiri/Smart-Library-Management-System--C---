#include "app/Services.hpp"

#include <stdexcept>

#include "utils/Hashing.hpp"
#include "utils/Time.hpp"

namespace libraflow::app {

AuthService::AuthService(db::UserRepository& users, db::AuditLogRepository& audit, utils::Logger& logger)
    : users_(users), audit_(audit), logger_(logger) {}

domain::User AuthService::register_user(const std::string& username, const std::string& password, domain::Role role) {
  if (username.empty() || password.size() < 6) {
    throw std::runtime_error("Username required and password must be at least 6 characters.");
  }
  domain::User user{};
  user.username = username;
  user.password_hash = utils::PasswordHasher::hash_password(password);
  user.role = role;
  user.created_at = utils::now_iso();
  const auto created = users_.create(user);
  audit_.log(created.id, "CREATE", "users", created.id);
  logger_.info("Registered user " + username);
  return created;
}

std::optional<domain::User> AuthService::login(const std::string& username, const std::string& password) {
  auto user = users_.find_by_username(username);
  if (!user.has_value()) return std::nullopt;
  if (!utils::PasswordHasher::verify_password(password, user->password_hash)) return std::nullopt;
  logger_.info("Login success for " + username);
  return user;
}

void AuthService::change_password(int user_id, const std::string& new_password) {
  if (new_password.size() < 6) {
    throw std::runtime_error("Password must be at least 6 characters.");
  }
  users_.update_password(user_id, utils::PasswordHasher::hash_password(new_password), false);
  audit_.log(user_id, "UPDATE", "users", user_id);
}

LibraryService::LibraryService(db::Database& database,
                               db::BookRepository& books,
                               db::MemberRepository& members,
                               db::LoanRepository& loans,
                               db::ReservationRepository& reservations,
                               db::AuditLogRepository& audit,
                               utils::Logger& logger,
                               utils::Config config)
    : database_(database),
      books_(books),
      members_(members),
      loans_(loans),
      reservations_(reservations),
      audit_(audit),
      logger_(logger),
      config_(std::move(config)) {}

std::vector<domain::Book> LibraryService::books(const std::string& query) {
  return query.empty() ? books_.list_all() : books_.search(query);
}

std::vector<domain::Member> LibraryService::members() {
  return members_.list_all();
}

std::vector<domain::Loan> LibraryService::loans() {
  return loans_.list_all();
}

std::vector<domain::Reservation> LibraryService::reservations() {
  return reservations_.list_all();
}

domain::Book LibraryService::add_book(const domain::Book& book, int actor_user_id) {
  if (book.total_copies < 1) throw std::runtime_error("Total copies must be positive.");
  auto payload = book;
  payload.available_copies = payload.total_copies;
  payload.created_at = utils::now_iso();
  const auto created = books_.create(payload);
  audit_.log(actor_user_id, "CREATE", "books", created.id);
  return created;
}

void LibraryService::update_book(const domain::Book& book, int actor_user_id) {
  books_.update(book);
  audit_.log(actor_user_id, "UPDATE", "books", book.id);
}

void LibraryService::delete_book(int book_id, int actor_user_id) {
  books_.remove(book_id);
  audit_.log(actor_user_id, "DELETE", "books", book_id);
}

domain::Member LibraryService::add_member(const domain::Member& member, int actor_user_id) {
  auto payload = member;
  payload.created_at = utils::now_iso();
  const auto created = members_.create(payload);
  audit_.log(actor_user_id, "CREATE", "members", created.id);
  return created;
}

void LibraryService::update_member(const domain::Member& member, int actor_user_id) {
  members_.update(member);
  audit_.log(actor_user_id, "UPDATE", "members", member.id);
}

void LibraryService::delete_member(int member_id, int actor_user_id) {
  members_.remove(member_id);
  audit_.log(actor_user_id, "DELETE", "members", member_id);
}

domain::Loan LibraryService::borrow_book(int book_id, int member_id, int actor_user_id, int due_days) {
  auto book = books_.find_by_id(book_id);
  if (!book.has_value()) throw std::runtime_error("Book not found.");
  if (!members_.find_by_id(member_id).has_value()) throw std::runtime_error("Member not found.");
  if (book->available_copies <= 0) throw std::runtime_error("Book unavailable. Use reservations.");

  domain::Loan created{};
  database_.transaction([&](sqlite3*) {
    books_.update_availability(book_id, book->available_copies - 1);
    domain::Loan loan{};
    loan.book_id = book_id;
    loan.member_id = member_id;
    loan.borrowed_at = utils::now_iso();
    loan.due_at = utils::add_days_iso(due_days);
    loan.status = domain::LoanStatus::Borrowed;
    created = loans_.create(loan);
  });
  audit_.log(actor_user_id, "BORROW", "loans", created.id);
  return loans_.find_by_id(created.id).value_or(created);
}

domain::Loan LibraryService::return_book(int loan_id, int actor_user_id) {
  auto loan = loans_.find_by_id(loan_id);
  if (!loan.has_value() || loan->returned_at.has_value()) throw std::runtime_error("Loan is not active.");
  auto book = books_.find_by_id(loan->book_id);
  if (!book.has_value()) throw std::runtime_error("Book not found.");

  const auto returned_at = utils::now_iso();
  const auto overdue_days = utils::days_overdue(loan->due_at, returned_at);
  const double fine = static_cast<double>(overdue_days) * config_.fine_per_day;

  database_.transaction([&](sqlite3*) {
    loans_.update_return(loan_id, returned_at, fine, domain::LoanStatus::Returned);
    books_.update_availability(book->id, book->available_copies + 1);
    auto queue = reservations_.list_pending_for_book(book->id);
    if (!queue.empty()) reservations_.update_status(queue.front().id, domain::ReservationStatus::ReadyForPickup);
  });
  audit_.log(actor_user_id, "RETURN", "loans", loan_id);
  return loans_.find_by_id(loan_id).value();
}

domain::Loan LibraryService::renew_loan(int loan_id, int actor_user_id, int extra_days) {
  auto loan = loans_.find_by_id(loan_id);
  if (!loan.has_value() || loan->returned_at.has_value()) throw std::runtime_error("Loan is not active.");
  if (!reservations_.list_pending_for_book(loan->book_id).empty()) {
    throw std::runtime_error("Loan cannot be renewed while reservations are pending.");
  }
  const auto new_due_date = utils::format_time_point(utils::parse_iso(loan->due_at) + std::chrono::hours(24 * extra_days));
  loans_.update_due_date(loan_id, new_due_date);
  audit_.log(actor_user_id, "RENEW", "loans", loan_id);
  return loans_.find_by_id(loan_id).value();
}

domain::Reservation LibraryService::reserve_book(int book_id, int member_id, int actor_user_id) {
  if (!books_.find_by_id(book_id).has_value()) throw std::runtime_error("Book not found.");
  if (!members_.find_by_id(member_id).has_value()) throw std::runtime_error("Member not found.");
  if (reservations_.has_active_reservation(book_id, member_id)) throw std::runtime_error("Reservation already exists.");
  const auto queue = reservations_.list_pending_for_book(book_id);
  domain::Reservation reservation{};
  reservation.book_id = book_id;
  reservation.member_id = member_id;
  reservation.reserved_at = utils::now_iso();
  reservation.status = domain::ReservationStatus::Pending;
  reservation.queue_position = static_cast<int>(queue.size()) + 1;
  const auto created = reservations_.create(reservation);
  audit_.log(actor_user_id, "RESERVE", "reservations", created.id);
  return created;
}

ReportService::ReportService(db::LoanRepository& loans, utils::Logger& logger) : loans_(loans), logger_(logger) {}

std::vector<domain::Loan> ReportService::overdue_loans() {
  return loans_.list_overdue();
}

std::vector<domain::MostBorrowedRow> ReportService::most_borrowed(int limit) {
  return loans_.most_borrowed(limit);
}

BackgroundScanner::BackgroundScanner(db::LoanRepository& loans, utils::Config config, utils::Logger& logger)
    : loans_(loans), config_(std::move(config)), logger_(logger) {}

BackgroundScanner::~BackgroundScanner() {
  stop();
}

void BackgroundScanner::start() {
  if (running_.exchange(true)) return;
  worker_ = std::thread([this]() {
    while (running_) {
      try {
        scan_once();
      } catch (const std::exception& ex) {
        logger_.error(std::string("Scanner failed: ") + ex.what());
      }
      for (int i = 0; i < config_.scanner_interval_seconds && running_; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  });
}

void BackgroundScanner::stop() {
  running_ = false;
  if (worker_.joinable()) worker_.join();
}

std::string BackgroundScanner::last_run_time() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_run_time_;
}

void BackgroundScanner::scan_once() {
  const auto all_loans = loans_.list_all();
  const auto now = utils::now_iso();
  for (const auto& loan : all_loans) {
    if (loan.returned_at.has_value()) continue;
    const auto overdue_days = utils::days_overdue(loan.due_at, now);
    const auto status = overdue_days > 0 ? domain::LoanStatus::Overdue : domain::LoanStatus::Borrowed;
    loans_.update_fine_and_status(loan.id, static_cast<double>(overdue_days) * config_.fine_per_day, status);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    last_run_time_ = now;
  }
  logger_.info("Overdue scan completed at " + now);
}

AppContext::AppContext(db::Database& database, utils::Config config_value, utils::Logger& logger_value)
    : users(database),
      books(database),
      members(database),
      loans(database),
      reservations_repo(database),
      audit(database),
      dashboard(database),
      auth(users, audit, logger_value),
      library(database, books, members, loans, reservations_repo, audit, logger_value, config_value),
      reports(loans, logger_value),
      scanner(loans, config_value, logger_value),
      config(std::move(config_value)),
      logger(logger_value),
      database_(database) {}

bool can_manage_catalog(domain::Role role) {
  return role == domain::Role::Admin || role == domain::Role::Librarian;
}

bool can_view_reports(domain::Role role) {
  return can_manage_catalog(role);
}

bool is_admin(domain::Role role) {
  return role == domain::Role::Admin;
}

}  // namespace libraflow::app
