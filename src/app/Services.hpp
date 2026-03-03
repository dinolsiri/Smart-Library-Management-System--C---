#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "db/Repositories.hpp"
#include "utils/Config.hpp"
#include "utils/Logger.hpp"

namespace libraflow::app {

struct Session {
  std::optional<domain::User> current_user;
};

class AuthService {
 public:
  AuthService(db::UserRepository& users, db::AuditLogRepository& audit, utils::Logger& logger);
  domain::User register_user(const std::string& username, const std::string& password, domain::Role role);
  std::optional<domain::User> login(const std::string& username, const std::string& password);
  void change_password(int user_id, const std::string& new_password);

 private:
  db::UserRepository& users_;
  db::AuditLogRepository& audit_;
  utils::Logger& logger_;
};

class LibraryService {
 public:
  LibraryService(db::Database& database,
                 db::BookRepository& books,
                 db::MemberRepository& members,
                 db::LoanRepository& loans,
                 db::ReservationRepository& reservations,
                 db::AuditLogRepository& audit,
                 utils::Logger& logger,
                 utils::Config config);
  std::vector<domain::Book> books(const std::string& query = "");
  std::vector<domain::Member> members();
  std::vector<domain::Loan> loans();
  std::vector<domain::Reservation> reservations();
  domain::Book add_book(const domain::Book& book, int actor_user_id);
  void update_book(const domain::Book& book, int actor_user_id);
  void delete_book(int book_id, int actor_user_id);
  domain::Member add_member(const domain::Member& member, int actor_user_id);
  void update_member(const domain::Member& member, int actor_user_id);
  void delete_member(int member_id, int actor_user_id);
  domain::Loan borrow_book(int book_id, int member_id, int actor_user_id, int due_days);
  domain::Loan return_book(int loan_id, int actor_user_id);
  domain::Loan renew_loan(int loan_id, int actor_user_id, int extra_days);
  domain::Reservation reserve_book(int book_id, int member_id, int actor_user_id);

 private:
  db::Database& database_;
  db::BookRepository& books_;
  db::MemberRepository& members_;
  db::LoanRepository& loans_;
  db::ReservationRepository& reservations_;
  db::AuditLogRepository& audit_;
  utils::Logger& logger_;
  utils::Config config_;
};

class ReportService {
 public:
  ReportService(db::LoanRepository& loans, utils::Logger& logger);
  std::vector<domain::Loan> overdue_loans();
  std::vector<domain::MostBorrowedRow> most_borrowed(int limit);

 private:
  db::LoanRepository& loans_;
  utils::Logger& logger_;
};

class BackgroundScanner {
 public:
  BackgroundScanner(db::LoanRepository& loans, utils::Config config, utils::Logger& logger);
  ~BackgroundScanner();
  void start();
  void stop();
  [[nodiscard]] std::string last_run_time() const;
  void scan_once();

 private:
  db::LoanRepository& loans_;
  utils::Config config_;
  utils::Logger& logger_;
  std::atomic<bool> running_{false};
  std::thread worker_;
  mutable std::mutex mutex_;
  std::string last_run_time_{"never"};
};

class AppContext {
 public:
  AppContext(db::Database& database, utils::Config config, utils::Logger& logger);

  Session session;
  db::UserRepository users;
  db::BookRepository books;
  db::MemberRepository members;
  db::LoanRepository loans;
  db::ReservationRepository reservations_repo;
  db::AuditLogRepository audit;
  db::DashboardRepository dashboard;
  AuthService auth;
  LibraryService library;
  ReportService reports;
  BackgroundScanner scanner;
  utils::Config config;
  utils::Logger& logger;

 private:
  db::Database& database_;
};

bool can_manage_catalog(domain::Role role);
bool can_view_reports(domain::Role role);
bool is_admin(domain::Role role);

}  // namespace libraflow::app
