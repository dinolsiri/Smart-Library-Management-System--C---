#pragma once

#include <optional>
#include <string>
#include <vector>

#include "db/Database.hpp"
#include "domain/Models.hpp"

namespace libraflow::db {

class UserRepository {
 public:
  explicit UserRepository(Database& database) : database_(database) {}
  domain::User create(const domain::User& user);
  std::optional<domain::User> find_by_username(const std::string& username);
  std::optional<domain::User> find_by_id(int id);
  std::vector<domain::User> list_all();
  void update_password(int id, const std::string& password_hash, bool must_change_password);

 private:
  Database& database_;
};

class BookRepository {
 public:
  explicit BookRepository(Database& database) : database_(database) {}
  domain::Book create(const domain::Book& book);
  void update(const domain::Book& book);
  void remove(int id);
  std::optional<domain::Book> find_by_id(int id);
  std::vector<domain::Book> search(const std::string& query);
  std::vector<domain::Book> list_all();
  void update_availability(int id, int available_copies);
  int total_count();

 private:
  Database& database_;
};

class MemberRepository {
 public:
  explicit MemberRepository(Database& database) : database_(database) {}
  domain::Member create(const domain::Member& member);
  void update(const domain::Member& member);
  void remove(int id);
  std::optional<domain::Member> find_by_id(int id);
  std::vector<domain::Member> list_all();
  int total_count();

 private:
  Database& database_;
};

class LoanRepository {
 public:
  explicit LoanRepository(Database& database) : database_(database) {}
  domain::Loan create(const domain::Loan& loan);
  void update_return(int loan_id, const std::string& returned_at, double fine_amount, domain::LoanStatus status);
  void update_due_date(int loan_id, const std::string& due_at);
  void update_fine_and_status(int loan_id, double fine_amount, domain::LoanStatus status);
  std::optional<domain::Loan> find_active_by_book_member(int book_id, int member_id);
  std::optional<domain::Loan> find_by_id(int loan_id);
  std::vector<domain::Loan> list_all();
  std::vector<domain::Loan> list_overdue();
  int active_count();
  int overdue_count();
  std::vector<domain::MostBorrowedRow> most_borrowed(int limit);

 private:
  Database& database_;
};

class ReservationRepository {
 public:
  explicit ReservationRepository(Database& database) : database_(database) {}
  domain::Reservation create(const domain::Reservation& reservation);
  std::vector<domain::Reservation> list_all();
  std::vector<domain::Reservation> list_pending_for_book(int book_id);
  void update_status(int reservation_id, domain::ReservationStatus status);
  int count_all();
  bool has_active_reservation(int book_id, int member_id);

 private:
  Database& database_;
};

class AuditLogRepository {
 public:
  explicit AuditLogRepository(Database& database) : database_(database) {}
  void log(int actor_user_id, const std::string& action, const std::string& entity, int entity_id);

 private:
  Database& database_;
};

class DashboardRepository {
 public:
  explicit DashboardRepository(Database& database) : database_(database) {}
  domain::DashboardStats stats();

 private:
  Database& database_;
};

}  // namespace libraflow::db
