#pragma once

#include <optional>
#include <string>

namespace libraflow::domain {

enum class Role { Admin, Librarian, Member };
enum class LoanStatus { Borrowed, Returned, Overdue };
enum class ReservationStatus { Pending, ReadyForPickup, Fulfilled, Cancelled };

struct User {
  int id{};
  std::string username;
  std::string password_hash;
  Role role{Role::Member};
  std::string created_at;
  bool must_change_password{false};
};

struct Book {
  int id{};
  std::string isbn;
  std::string title;
  std::string author;
  std::string category;
  int year{};
  int total_copies{};
  int available_copies{};
  std::string created_at;
};

struct Member {
  int id{};
  std::string member_no;
  std::string name;
  std::string email;
  std::string phone;
  std::string created_at;
};

struct Loan {
  int id{};
  int book_id{};
  int member_id{};
  std::string borrowed_at;
  std::string due_at;
  std::optional<std::string> returned_at;
  double fine_amount{};
  LoanStatus status{LoanStatus::Borrowed};
  std::string book_title;
  std::string member_name;
};

struct Reservation {
  int id{};
  int book_id{};
  int member_id{};
  std::string reserved_at;
  ReservationStatus status{ReservationStatus::Pending};
  int queue_position{};
  std::string book_title;
  std::string member_name;
};

struct DashboardStats {
  int total_books{};
  int total_members{};
  int active_loans{};
  int overdue_loans{};
  int reservations{};
};

struct MostBorrowedRow {
  std::string title;
  int borrow_count{};
};

inline std::string to_string(Role role) {
  switch (role) {
    case Role::Admin: return "Admin";
    case Role::Librarian: return "Librarian";
    case Role::Member: return "Member";
  }
  return "Member";
}

inline std::string to_string(LoanStatus status) {
  switch (status) {
    case LoanStatus::Borrowed: return "BORROWED";
    case LoanStatus::Returned: return "RETURNED";
    case LoanStatus::Overdue: return "OVERDUE";
  }
  return "BORROWED";
}

inline std::string to_string(ReservationStatus status) {
  switch (status) {
    case ReservationStatus::Pending: return "PENDING";
    case ReservationStatus::ReadyForPickup: return "READY";
    case ReservationStatus::Fulfilled: return "FULFILLED";
    case ReservationStatus::Cancelled: return "CANCELLED";
  }
  return "PENDING";
}

inline Role role_from_string(const std::string& role) {
  if (role == "Admin") return Role::Admin;
  if (role == "Librarian") return Role::Librarian;
  return Role::Member;
}

inline LoanStatus loan_status_from_string(const std::string& status) {
  if (status == "Returned" || status == "RETURNED") return LoanStatus::Returned;
  if (status == "Overdue" || status == "OVERDUE") return LoanStatus::Overdue;
  return LoanStatus::Borrowed;
}

inline ReservationStatus reservation_status_from_string(const std::string& status) {
  if (status == "ReadyForPickup" || status == "READY") return ReservationStatus::ReadyForPickup;
  if (status == "Fulfilled" || status == "FULFILLED") return ReservationStatus::Fulfilled;
  if (status == "Cancelled" || status == "CANCELLED") return ReservationStatus::Cancelled;
  return ReservationStatus::Pending;
}

}  // namespace libraflow::domain
