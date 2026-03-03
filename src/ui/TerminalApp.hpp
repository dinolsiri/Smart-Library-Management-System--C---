#pragma once

#include <functional>
#include <string>

#include "app/Services.hpp"

namespace libraflow::ui {

class TerminalApp {
 public:
  explicit TerminalApp(app::AppContext& context);
  void run();

 private:
  enum class Screen {
    Login,
    Dashboard,
    Books,
    Members,
    Loans,
    Reservations,
    Reports,
    Settings
  };

  app::AppContext& context_;
  std::string toast_message_;
  bool toast_error_{false};
  std::string login_username_{"admin"};
  std::string login_password_{"admin123"};
  std::string register_username_;
  std::string register_password_;
  int register_role_index_{2};
  std::string book_query_;
  std::string book_isbn_;
  std::string book_title_;
  std::string book_author_;
  std::string book_category_;
  std::string book_year_{"2024"};
  std::string book_copies_{"1"};
  std::string book_edit_id_;
  std::string book_delete_id_;
  std::string member_no_;
  std::string member_name_;
  std::string member_email_;
  std::string member_phone_;
  std::string member_edit_id_;
  std::string member_delete_id_;
  std::string loan_book_id_;
  std::string loan_member_id_;
  std::string loan_due_days_{"14"};
  std::string return_loan_id_;
  std::string renew_loan_id_;
  std::string renew_days_{"7"};
  std::string reserve_book_id_;
  std::string reserve_member_id_;
  std::string report_top_n_{"5"};
  std::string change_password_;
  int nav_selected_{0};
  Screen current_screen_{Screen::Login};
  bool confirm_visible_{false};
  std::string confirm_title_;
  std::string confirm_body_;
  std::function<void()> confirm_action_;

  void set_toast(const std::string& message, bool error = false);
  void login();
  void register_user();
  void logout();
  void show_confirm(const std::string& title, const std::string& body, std::function<void()> action);
};

}  // namespace libraflow::ui
