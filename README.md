# LibraFlow

LibraFlow is a smart library management system written in modern C++20 with SQLite persistence and an FTXUI-powered terminal interface. It provides authentication, role-based access, book/member management, loans, reservations, reports, audit logging, and a background overdue scanner.

## Features

- C++20 + CMake project structure with layered architecture
- SQLite file database with migrations and indexes
- Futuristic terminal UI using FTXUI
- Roles: `Admin`, `Librarian`, `Member`
- Password hashing with random salt and forced password change for the seeded admin
- Books, members, loans, reservations, reports, CSV export
- Background thread for overdue scanning and fine updates
- Logging, audit trail, and a minimal test target
LIBRAFLOW Smart Library Management System Modern C++20 | SQLite | FTXUI
Terminal UI

============================================================

## OVERVIEW

LibraFlow is a high-performance, terminal-based Library Management
System built using Modern C++20. It combines clean layered architecture,
SQLite persistence, and a futuristic FTXUI-powered interface to deliver
a production-style academic project suitable for professional
portfolios.

The system is designed with scalability, maintainability, and real-world
software engineering principles in mind.

============================================================

## KEY HIGHLIGHTS

-   Modern C++20 implementation
-   Clean layered architecture (UI -> App -> Domain -> DB -> Utils)
-   SQLite persistent database with automatic migrations
-   Futuristic terminal interface built with FTXUI
-   Role-based authentication (Admin / Librarian / Member)
-   Secure password hashing with salted storage
-   Background overdue scanner using multithreading
-   Fine calculation engine
-   Reservation queue system
-   CSV report export
-   Audit logging and activity tracking
-   Configurable settings via config.json

============================================================

## SYSTEM ARCHITECTURE

src/ app/ Business logic and service orchestration db/ SQLite wrapper,
migrations, repositories domain/ Core entities and enums ui/ FTXUI
terminal screens and navigation utils/ Configuration, logging, hashing,
time, CSV tests/ Minimal unit tests

Design Principles Used: - SOLID principles - Repository pattern -
Role-based access control - Prepared SQL statements - Thread-safe
background processing - Separation of concerns

============================================================

## CORE FEATURES

Authentication & Authorization - Role-based login system - Salted
password hashing - Forced password change for default admin -
Permission-restricted UI access

Book Management - Add, edit, delete books - Search by title, author,
ISBN, category - Indexed queries for fast retrieval - Availability
tracking

Member Management - Register and manage library members - Unique member
IDs - Contact information tracking

Loans & Returns - Borrow and return workflow - Due date management -
Automatic fine calculation - Renewal support

Reservation System - Queue-based reservations - Automatic pickup
availability - Status tracking

Reports - Overdue loan listing - Most borrowed books - CSV export to
exports/ directory

Background Services - Periodic overdue scanner (runs in separate
thread) - Automatic fine updates - Status bar shows last scan time

Logging & Auditing - Application logs written to libraflow.log - Audit
trail records major actions - Structured error handling

============================================================

## DATABASE SCHEMA

LibraFlow automatically initializes and migrates the SQLite database on
first run.

Main tables: - users - books - members - loans - reservations -
audit_logs

Search and overdue operations are optimized using indexes.

============================================================

## PREREQUISITES

Windows: - CMake 3.21+ - Visual Studio 2022 Build Tools OR MinGW (C++20
compatible) - Git

Linux: - CMake 3.21+ - GCC 11+ or Clang 14+ - Make or Ninja - Git

Dependencies such as FTXUI, SQLite, and JSON libraries are automatically
fetched via CMake during configuration.

============================================================

## BUILD INSTRUCTIONS

Windows: cmake -S . -B build cmake –build build –config Release

Linux: cmake -S . -B build cmake –build build -j

============================================================

## RUNNING THE APPLICATION

Windows: ..exe

Linux: ./build/LibraFlow

============================================================

## DEFAULT ADMIN ACCOUNT

On first launch, the system seeds a default admin:

Username: admin Password: admin123

The admin is required to change the password after the first login.

============================================================

## KEYBOARD CONTROLS

Arrow Keys / Tab - Navigate Enter - Select / Confirm Esc - Close dialog
Ctrl + Q - Exit application

============================================================

## CONFIGURATION

The config.json file controls: - Database file location - Fine per day
amount - Overdue scan interval - Export directory path

Modify this file to customize runtime behavior.

============================================================

## TESTING

cmake –build build –target LibraFlowTests ./build/LibraFlowTests

============================================================

## PRODUCTION VALUE

This project demonstrates: - Concurrency control in database-driven
systems - Secure credential storage practices - Practical terminal UI
design - Structured C++ application architecture - Real-world CRUD +
workflow logic - Persistent storage with migration handling

LibraFlow is suitable as: - Final-year academic project - Portfolio
showcase - Systems programming demonstration - Software architecture
reference

============================================================

## FUTURE IMPROVEMENTS

-   REST API wrapper (Drogon-based backend)
-   Web dashboard frontend
-   Docker containerization
-   Advanced analytics dashboard
-   Barcode/QR code integration
-   Email notification module
-   Multi-database support (PostgreSQL)

============================================================

License: Educational / Portfolio Use

## Project layout

```text
src/
  app/       Application services and orchestration
  db/        SQLite wrapper, migrations, repositories
  domain/    Entities and enums
  ui/        FTXUI screens and terminal shell
  utils/     Config, time, hashing, CSV, logging
tests/       Minimal test runner
```

## Prerequisites

### Windows

- CMake 3.21+
- Visual Studio 2022 Build Tools or MinGW with C++20 support
- Git

### Linux

- CMake 3.21+
- GCC 11+ or Clang 14+
- Git
- `make` or Ninja

`FTXUI`, `nlohmann/json`, and SQLite are fetched automatically by CMake during configure.

## Build

### Windows

```powershell
cmake -S . -B build
cmake --build build --config Release
```

### Linux

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

### Windows

```powershell
.\build\Release\LibraFlow.exe
```

If you used a single-config generator:

```powershell
.\build\LibraFlow.exe
```

### Linux

```bash
./build/LibraFlow
```

## Default admin flow

On first run, migrations create the database and seed:

- Username: `admin`
- Password: `admin123`

The seeded admin is forced to change the password on the first login from the `Settings` screen.

## Controls

- `Arrow keys` or `Tab`: navigate
- `Enter`: activate focused action
- `Esc`: dismiss notification
- `Ctrl+Q`: quit

## Reports

Use the `Reports` screen to view overdue loans, most borrowed books, and export CSV files to `exports/`.

## Tests

```bash
cmake --build build --target LibraFlowTests
./build/LibraFlowTests
```

On Windows:

```powershell
cmake --build build --config Release --target LibraFlowTests
.\build\Release\LibraFlowTests.exe
```

## Notes

- `config.json` controls database path, fine amount, and background scan interval.
- The app writes logs to `libraflow.log`.
- The overdue scanner updates loan statuses and fine amounts in the background. The last run time is shown in the status bar.

## Author
Dinol siriwardena
https://github.com/dinolsiri