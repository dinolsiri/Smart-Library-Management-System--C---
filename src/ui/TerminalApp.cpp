#include "ui/TerminalApp.hpp"

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "utils/CsvExporter.hpp"

namespace libraflow::ui {

using namespace ftxui;

namespace {

Color kBg = Color::RGB(11, 15, 28);
Color kPanel = Color::RGB(18, 30, 49);
Color kAccent = Color::RGB(0, 255, 214);
Color kAccent2 = Color::RGB(255, 0, 153);
Color kText = Color::RGB(191, 255, 247);
Color kMuted = Color::RGB(122, 163, 176);
Color kWarn = Color::RGB(255, 196, 0);
Color kError = Color::RGB(255, 84, 112);
Color kSuccess = Color::RGB(76, 255, 122);

std::vector<std::string> roles = {"Admin", "Librarian", "Member"};

Element cyber_header() {
  return vbox({
             text(" _     _ _               _____ _                 "),
             text("| |   (_) |__  _ __ __ _|  ___| | _____      __  "),
             text("| |   | | '_ \\| '__/ _` | |_  | |/ _ \\ \\ /\\ / /  "),
             text("| |___| | |_) | | | (_| |  _| | | (_) \\ V  V /   "),
             text("|_____|_|_.__/|_|  \\__,_|_|   |_|\\___/ \\_/\\_/    "),
         }) |
         color(kAccent) | bold;
}

Element neon_box(const std::string& title, Element body) {
  return window(text(" " + title + " ") | color(kAccent2) | bold, body | color(kText) | bgcolor(kPanel)) | color(kAccent);
}

Element badge(const std::string& label) {
  Color color = kAccent;
  if (label == "BORROWED") color = kWarn;
  if (label == "OVERDUE") color = kError;
  if (label == "AVAILABLE") color = kSuccess;
  if (label == "READY") color = kAccent2;
  return text(" " + label + " ") | bgcolor(color) | color(Color::Black);
}

std::string role_badge(libraflow::domain::Role role) {
  return "[" + libraflow::domain::to_string(role) + "]";
}

std::string book_badge(const libraflow::domain::Book& book) {
  return book.available_copies > 0 ? "AVAILABLE" : "BORROWED";
}

std::string loan_badge(const libraflow::domain::Loan& loan) {
  return libraflow::domain::to_string(loan.status);
}

int parse_int(const std::string& value) {
  return std::stoi(value);
}

}  // namespace

TerminalApp::TerminalApp(app::AppContext& context) : context_(context) {}

void TerminalApp::set_toast(const std::string& message, bool error) {
  toast_message_ = message;
  toast_error_ = error;
}

void TerminalApp::login() {
  auto user = context_.auth.login(login_username_, login_password_);
  if (!user.has_value()) {
    set_toast("Login failed", true);
    return;
  }
  context_.session.current_user = user;
  current_screen_ = user->must_change_password ? Screen::Settings : Screen::Dashboard;
  set_toast("Welcome " + user->username);
}

void TerminalApp::register_user() {
  if (!context_.session.current_user.has_value() || !app::is_admin(context_.session.current_user->role)) {
    set_toast("Only admins can register users", true);
    return;
  }
  try {
    context_.auth.register_user(register_username_, register_password_, libraflow::domain::role_from_string(roles[register_role_index_]));
    set_toast("User created");
    register_username_.clear();
    register_password_.clear();
  } catch (const std::exception& ex) {
    set_toast(ex.what(), true);
  }
}

void TerminalApp::logout() {
  context_.session.current_user.reset();
  current_screen_ = Screen::Login;
  set_toast("Logged out");
}

void TerminalApp::show_confirm(const std::string& title, const std::string& body, std::function<void()> action) {
  confirm_title_ = title;
  confirm_body_ = body;
  confirm_action_ = std::move(action);
  confirm_visible_ = true;
}

void TerminalApp::run() {
  context_.scanner.start();
  auto screen = ScreenInteractive::Fullscreen();
  std::atomic<int> spinner{0};
  std::atomic<bool> ticking{true};
  std::thread ticker([&]() {
    while (ticking) {
      std::this_thread::sleep_for(std::chrono::milliseconds(120));
      spinner = (spinner + 1) % 4;
      screen.PostEvent(Event::Custom);
    }
  });

  std::vector<std::string> nav_labels = {"Dashboard", "Books", "Members", "Loans", "Reservations", "Reports", "Settings"};
  auto nav_menu = Menu(&nav_labels, &nav_selected_);
  auto login_user_input = Input(&login_username_, "username");
  auto login_pass_input = Input(&login_password_, "password");
  auto login_button = Button("Login", [&] { login(); });
  auto reg_user_input = Input(&register_username_, "new username");
  auto reg_pass_input = Input(&register_password_, "new password");
  auto role_menu = Menu(&roles, &register_role_index_);
  auto register_button = Button("Create User", [&] { register_user(); });
  auto book_query_input = Input(&book_query_, "search books");
  auto book_isbn_input = Input(&book_isbn_, "isbn");
  auto book_title_input = Input(&book_title_, "title");
  auto book_author_input = Input(&book_author_, "author");
  auto book_category_input = Input(&book_category_, "category");
  auto book_year_input = Input(&book_year_, "year");
  auto book_copies_input = Input(&book_copies_, "copies");
  auto book_edit_input = Input(&book_edit_id_, "edit id");
  auto book_delete_input = Input(&book_delete_id_, "delete id");
  auto add_book_button = Button("Add Book", [&] {
    try {
      libraflow::domain::Book book{};
      book.isbn = book_isbn_;
      book.title = book_title_;
      book.author = book_author_;
      book.category = book_category_;
      book.year = parse_int(book_year_);
      book.total_copies = parse_int(book_copies_);
      context_.library.add_book(book, context_.session.current_user->id);
      set_toast("Book added");
      book_isbn_.clear();
      book_title_.clear();
      book_author_.clear();
      book_category_.clear();
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto update_book_button = Button("Update Book", [&] {
    try {
      libraflow::domain::Book book{};
      book.id = parse_int(book_edit_id_);
      book.isbn = book_isbn_;
      book.title = book_title_;
      book.author = book_author_;
      book.category = book_category_;
      book.year = parse_int(book_year_);
      book.total_copies = parse_int(book_copies_);
      auto existing = context_.books.find_by_id(book.id);
      if (!existing.has_value()) throw std::runtime_error("Book not found.");
      book.available_copies = std::min(existing->available_copies, book.total_copies);
      context_.library.update_book(book, context_.session.current_user->id);
      set_toast("Book updated");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto delete_book_button = Button("Delete Book", [&] {
    try {
      const int book_id = parse_int(book_delete_id_);
      show_confirm("Delete Book",
                   "Delete book id " + std::to_string(book_id) + "?",
                   [&, book_id]() {
                     try {
                       context_.library.delete_book(book_id, context_.session.current_user->id);
                       set_toast("Book deleted");
                     } catch (const std::exception& ex) {
                       set_toast(ex.what(), true);
                     }
                   });
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto member_no_input = Input(&member_no_, "member no");
  auto member_name_input = Input(&member_name_, "member name");
  auto member_email_input = Input(&member_email_, "email");
  auto member_phone_input = Input(&member_phone_, "phone");
  auto member_edit_input = Input(&member_edit_id_, "edit id");
  auto member_delete_input = Input(&member_delete_id_, "delete id");
  auto add_member_button = Button("Add Member", [&] {
    try {
      libraflow::domain::Member member{};
      member.member_no = member_no_;
      member.name = member_name_;
      member.email = member_email_;
      member.phone = member_phone_;
      context_.library.add_member(member, context_.session.current_user->id);
      set_toast("Member added");
      member_no_.clear();
      member_name_.clear();
      member_email_.clear();
      member_phone_.clear();
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto update_member_button = Button("Update Member", [&] {
    try {
      libraflow::domain::Member member{};
      member.id = parse_int(member_edit_id_);
      member.member_no = member_no_;
      member.name = member_name_;
      member.email = member_email_;
      member.phone = member_phone_;
      context_.library.update_member(member, context_.session.current_user->id);
      set_toast("Member updated");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto delete_member_button = Button("Delete Member", [&] {
    try {
      const int member_id = parse_int(member_delete_id_);
      show_confirm("Delete Member",
                   "Delete member id " + std::to_string(member_id) + "?",
                   [&, member_id]() {
                     try {
                       context_.library.delete_member(member_id, context_.session.current_user->id);
                       set_toast("Member deleted");
                     } catch (const std::exception& ex) {
                       set_toast(ex.what(), true);
                     }
                   });
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto loan_book_input = Input(&loan_book_id_, "book id");
  auto loan_member_input = Input(&loan_member_id_, "member id");
  auto loan_due_input = Input(&loan_due_days_, "due days");
  auto borrow_button = Button("Borrow", [&] {
    try {
      context_.library.borrow_book(parse_int(loan_book_id_), parse_int(loan_member_id_), context_.session.current_user->id, parse_int(loan_due_days_));
      set_toast("Loan created");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto return_input = Input(&return_loan_id_, "loan id");
  auto return_button = Button("Return", [&] {
    try {
      context_.library.return_book(parse_int(return_loan_id_), context_.session.current_user->id);
      set_toast("Loan returned");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto renew_input = Input(&renew_loan_id_, "loan id");
  auto renew_days_input = Input(&renew_days_, "extra days");
  auto renew_button = Button("Renew", [&] {
    try {
      context_.library.renew_loan(parse_int(renew_loan_id_), context_.session.current_user->id, parse_int(renew_days_));
      set_toast("Loan renewed");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto reserve_book_input = Input(&reserve_book_id_, "book id");
  auto reserve_member_input = Input(&reserve_member_id_, "member id");
  auto reserve_button = Button("Reserve", [&] {
    try {
      context_.library.reserve_book(parse_int(reserve_book_id_), parse_int(reserve_member_id_), context_.session.current_user->id);
      set_toast("Reservation created");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto top_n_input = Input(&report_top_n_, "top n");
  auto export_overdue_button = Button("Export Overdue CSV", [&] {
    auto rows = context_.reports.overdue_loans();
    std::vector<std::vector<std::string>> csv_rows;
    for (const auto& row : rows) csv_rows.push_back({std::to_string(row.id), row.book_title, row.member_name, row.due_at, std::to_string(row.fine_amount)});
    utils::CsvExporter::export_rows("exports/overdue.csv", {"Loan ID", "Book", "Member", "Due At", "Fine"}, csv_rows);
    set_toast("Exported exports/overdue.csv");
  });
  auto export_top_button = Button("Export Top Books CSV", [&] {
    auto rows = context_.reports.most_borrowed(parse_int(report_top_n_));
    std::vector<std::vector<std::string>> csv_rows;
    for (const auto& row : rows) csv_rows.push_back({row.title, std::to_string(row.borrow_count)});
    utils::CsvExporter::export_rows("exports/top_books.csv", {"Title", "Borrow Count"}, csv_rows);
    set_toast("Exported exports/top_books.csv");
  });
  auto change_password_input = Input(&change_password_, "new password");
  auto change_password_button = Button("Change Password", [&] {
    try {
      context_.auth.change_password(context_.session.current_user->id, change_password_);
      context_.session.current_user = context_.users.find_by_id(context_.session.current_user->id);
      change_password_.clear();
      set_toast("Password updated");
    } catch (const std::exception& ex) {
      set_toast(ex.what(), true);
    }
  });
  auto confirm_yes_button = Button("Confirm", [&] {
    if (confirm_action_) confirm_action_();
    confirm_visible_ = false;
    confirm_action_ = {};
  });
  auto confirm_no_button = Button("Cancel", [&] {
    confirm_visible_ = false;
    confirm_action_ = {};
  });
  auto logout_button = Button("Logout", [&] { logout(); });

  auto container = Container::Vertical({
      login_user_input, login_pass_input, login_button, nav_menu,
      book_query_input, book_isbn_input, book_title_input, book_author_input, book_category_input, book_year_input, book_copies_input, book_edit_input, book_delete_input, add_book_button, update_book_button, delete_book_button,
      member_no_input, member_name_input, member_email_input, member_phone_input, member_edit_input, member_delete_input, add_member_button, update_member_button, delete_member_button,
      loan_book_input, loan_member_input, loan_due_input, borrow_button, return_input, return_button, renew_input, renew_days_input, renew_button,
      reserve_book_input, reserve_member_input, reserve_button,
      reg_user_input, reg_pass_input, role_menu, register_button,
      top_n_input, export_overdue_button, export_top_button,
      change_password_input, change_password_button, confirm_yes_button, confirm_no_button, logout_button,
  });

  auto renderer = Renderer(container, [&] {
    const auto loading_frames = std::vector<std::string>{"[=   ]", "[==  ]", "[=== ]", "[ ===]"};
    auto status = text(" " + loading_frames[spinner] + " Overdue scan: " + context_.scanner.last_run_time() + " ") | color(kMuted);

    if (!context_.session.current_user.has_value()) {
      return vbox({
                 filler(),
                 hbox({
                     filler(),
                     neon_box("LibraFlow Access",
                              vbox({
                                  cyber_header(),
                                  separator(),
                                  text("Username") | color(kMuted),
                                  login_user_input->Render(),
                                  text("Password") | color(kMuted),
                                  login_pass_input->Render(),
                                  login_button->Render(),
                                  text("Seeded admin: admin / admin123") | color(kWarn),
                              }) | size(WIDTH, LESS_THAN, 60)),
                     filler(),
                 }),
                 filler(),
                 status | border | color(kAccent),
             }) |
             bgcolor(kBg);
    }

    std::vector<std::string> allowed_nav = {"Dashboard", "Books", "Members", "Loans", "Reservations", "Settings"};
    if (context_.session.current_user->must_change_password) {
      allowed_nav = {"Settings"};
      nav_selected_ = 0;
    } else if (app::can_view_reports(context_.session.current_user->role)) {
      allowed_nav.insert(allowed_nav.begin() + 5, "Reports");
    }
    nav_labels = allowed_nav;
    if (nav_selected_ >= static_cast<int>(nav_labels.size())) nav_selected_ = 0;

    std::vector<Screen> screen_map = {Screen::Dashboard, Screen::Books, Screen::Members, Screen::Loans, Screen::Reservations};
    if (context_.session.current_user->must_change_password) {
      screen_map = {Screen::Settings};
    } else {
      if (app::can_view_reports(context_.session.current_user->role)) screen_map.push_back(Screen::Reports);
      screen_map.push_back(Screen::Settings);
    }
    current_screen_ = screen_map[nav_selected_];

    Element main_content = text("");
    if (current_screen_ == Screen::Dashboard) {
      auto stats = context_.dashboard.stats();
      main_content = neon_box("Dashboard", vbox({
                                              cyber_header(),
                                              separator(),
                                              text("Signed in as " + context_.session.current_user->username + " " + role_badge(context_.session.current_user->role)) | color(kAccent),
                                              separator(),
                                              text("Books       : " + std::to_string(stats.total_books)),
                                              text("Members     : " + std::to_string(stats.total_members)),
                                              text("Active Loans: " + std::to_string(stats.active_loans)),
                                              text("Overdue     : " + std::to_string(stats.overdue_loans)) | color(kWarn),
                                              text("Reservations: " + std::to_string(stats.reservations)),
                                          }));
    }
    if (current_screen_ == Screen::Books) {
      auto books = context_.library.books(book_query_);
      std::vector<Element> rows{hbox({text("ID  ISBN          TITLE / AUTHOR                         STATE") | color(kMuted)})};
      for (const auto& book : books) rows.push_back(hbox({text((std::to_string(book.id) + "   ").substr(0, 4)), text(book.isbn + "  ") | size(WIDTH, EQUAL, 15), text(book.title + " / " + book.author) | size(WIDTH, GREATER_THAN, 35), filler(), badge(book_badge(book))}));
      if (rows.size() == 1) rows.push_back(text("No books found.") | color(kMuted));
      auto body = vbox({text("Search") | color(kMuted), book_query_input->Render(), separator(), vbox(std::move(rows)) | frame | size(HEIGHT, LESS_THAN, 14)});
      if (app::can_manage_catalog(context_.session.current_user->role)) {
        body = vbox({
            body,
            separator(),
            text("Book Editor") | color(kAccent2),
            hbox({book_isbn_input->Render(), text(" "), book_title_input->Render()}),
            hbox({book_author_input->Render(), text(" "), book_category_input->Render()}),
            hbox({book_year_input->Render(), text(" "), book_copies_input->Render(), text(" "), add_book_button->Render()}),
            hbox({book_edit_input->Render(), text(" "), update_book_button->Render(), text(" "), book_delete_input->Render(), text(" "), delete_book_button->Render()}),
        });
      }
      main_content = neon_box("Books", body);
    }
    if (current_screen_ == Screen::Members) {
      auto members = context_.library.members();
      std::vector<Element> rows{text("ID  MEMBER NO     NAME                      EMAIL")};
      for (const auto& member : members) rows.push_back(hbox({text((std::to_string(member.id) + "   ").substr(0, 4)), text(member.member_no) | size(WIDTH, EQUAL, 14), text(member.name) | size(WIDTH, EQUAL, 26), text(member.email)}));
      if (rows.size() == 1) rows.push_back(text("No members found.") | color(kMuted));
      auto body = vbox({vbox(std::move(rows)) | frame | size(HEIGHT, LESS_THAN, 12)});
      if (app::can_manage_catalog(context_.session.current_user->role)) {
        body = vbox({
            body,
            separator(),
            text("Member Editor") | color(kAccent2),
            hbox({member_no_input->Render(), text(" "), member_name_input->Render()}),
            hbox({member_email_input->Render(), text(" "), member_phone_input->Render(), text(" "), add_member_button->Render()}),
            hbox({member_edit_input->Render(), text(" "), update_member_button->Render(), text(" "), member_delete_input->Render(), text(" "), delete_member_button->Render()}),
        });
      }
      main_content = neon_box("Members", body);
    }
    if (current_screen_ == Screen::Loans) {
      auto loans = context_.library.loans();
      std::vector<Element> rows{text("ID  BOOK                    MEMBER                  DUE                 STATUS")};
      for (const auto& loan : loans) rows.push_back(hbox({text((std::to_string(loan.id) + "   ").substr(0, 4)), text(loan.book_title) | size(WIDTH, EQUAL, 24), text(loan.member_name) | size(WIDTH, EQUAL, 22), text(loan.due_at) | size(WIDTH, EQUAL, 21), filler(), badge(loan_badge(loan))}));
      if (rows.size() == 1) rows.push_back(text("No loans found.") | color(kMuted));
      main_content = neon_box("Loans", vbox({vbox(std::move(rows)) | frame | size(HEIGHT, LESS_THAN, 12), separator(), text("Borrow / Return / Renew") | color(kAccent2), hbox({loan_book_input->Render(), text(" "), loan_member_input->Render(), text(" "), loan_due_input->Render(), text(" "), borrow_button->Render()}), hbox({return_input->Render(), text(" "), return_button->Render(), text(" "), renew_input->Render(), text(" "), renew_days_input->Render(), text(" "), renew_button->Render()})}));
    }
    if (current_screen_ == Screen::Reservations) {
      auto reservations = context_.library.reservations();
      std::vector<Element> rows{text("ID  BOOK                    MEMBER                  QUEUE  STATUS")};
      for (const auto& reservation : reservations) rows.push_back(hbox({text((std::to_string(reservation.id) + "   ").substr(0, 4)), text(reservation.book_title) | size(WIDTH, EQUAL, 24), text(reservation.member_name) | size(WIDTH, EQUAL, 22), text(std::to_string(reservation.queue_position)) | size(WIDTH, EQUAL, 7), filler(), badge(libraflow::domain::to_string(reservation.status))}));
      if (rows.size() == 1) rows.push_back(text("No reservations found.") | color(kMuted));
      main_content = neon_box("Reservations", vbox({vbox(std::move(rows)) | frame | size(HEIGHT, LESS_THAN, 12), separator(), text("Reserve Unavailable Book") | color(kAccent2), hbox({reserve_book_input->Render(), text(" "), reserve_member_input->Render(), text(" "), reserve_button->Render()})}));
    }
    if (current_screen_ == Screen::Reports) {
      auto overdue = context_.reports.overdue_loans();
      auto top = context_.reports.most_borrowed(parse_int(report_top_n_));
      std::vector<Element> overdue_rows{text("LOAN  BOOK                    MEMBER                  FINE")};
      for (const auto& row : overdue) overdue_rows.push_back(hbox({text((std::to_string(row.id) + "   ").substr(0, 5)), text(row.book_title) | size(WIDTH, EQUAL, 24), text(row.member_name) | size(WIDTH, EQUAL, 22), text(std::to_string(row.fine_amount))}));
      if (overdue_rows.size() == 1) overdue_rows.push_back(text("No overdue loans.") | color(kMuted));
      std::vector<Element> top_rows{text("TITLE                                      BORROWS")};
      for (const auto& row : top) top_rows.push_back(hbox({text(row.title) | size(WIDTH, EQUAL, 42), text(std::to_string(row.borrow_count))}));
      if (top_rows.size() == 1) top_rows.push_back(text("No borrow history yet.") | color(kMuted));
      main_content = neon_box("Reports", vbox({text("Overdue Loans") | color(kWarn), vbox(std::move(overdue_rows)) | frame | size(HEIGHT, LESS_THAN, 8), separator(), text("Most Borrowed Books") | color(kAccent2), hbox({text("Top N "), top_n_input->Render(), text(" "), export_top_button->Render(), text(" "), export_overdue_button->Render()}), vbox(std::move(top_rows)) | frame | size(HEIGHT, LESS_THAN, 8)}));
    }
    if (current_screen_ == Screen::Settings) {
      auto body = vbox({text("Change password") | color(kAccent2), change_password_input->Render(), change_password_button->Render(), separator(), text("Database: " + context_.config.db_path), text("Fine per day: " + std::to_string(context_.config.fine_per_day)), text("Scanner interval: " + std::to_string(context_.config.scanner_interval_seconds) + "s"), separator(), logout_button->Render()});
      if (app::is_admin(context_.session.current_user->role)) body = vbox({body, separator(), text("Admin: Create User") | color(kAccent2), reg_user_input->Render(), reg_pass_input->Render(), role_menu->Render(), register_button->Render()});
      if (context_.session.current_user->must_change_password) body = vbox({text("Password change required before continuing.") | color(kWarn) | bold, separator(), body});
      main_content = neon_box("Settings", body);
    }

    auto nav_panel = neon_box("Navigation", vbox({text("Use arrows / Tab / Enter") | color(kMuted), separator(), nav_menu->Render()}));
    auto status_bar = hbox({text(" Ctrl+Q quit ") | color(kBg) | bgcolor(kAccent), text(" Esc clear toast ") | color(kBg) | bgcolor(kAccent2), filler(), status}) | border;
    Element layout = vbox({hbox({nav_panel | size(WIDTH, EQUAL, 26), main_content | flex}) | flex, status_bar}) | bgcolor(kBg);
    if (!toast_message_.empty()) {
      layout = dbox({layout, vbox({filler(), hbox({filler(), neon_box("Notice", text(toast_message_) | color(toast_error_ ? kError : kSuccess)), filler()})})});
    }
    if (confirm_visible_) {
      layout = dbox({
          layout,
          vbox({
              filler(),
              hbox({
                  filler(),
                  neon_box(confirm_title_, vbox({text(confirm_body_), separator(), hbox({confirm_yes_button->Render(), text(" "), confirm_no_button->Render()})})),
                  filler(),
              }),
              filler(),
          }),
      });
    }
    return layout;
  });

  auto root = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Escape) {
      if (confirm_visible_) {
        confirm_visible_ = false;
        confirm_action_ = {};
        return true;
      }
      toast_message_.clear();
      return true;
    }
    if (event == Event::Character("\x11")) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
  ticking = false;
  if (ticker.joinable()) ticker.join();
  context_.scanner.stop();
}

}  // namespace libraflow::ui
