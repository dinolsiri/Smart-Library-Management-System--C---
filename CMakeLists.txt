cmake_minimum_required(VERSION 3.21)
project(LibraFlow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build LibraFlow tests" ON)

include(FetchContent)

set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v5.0.0
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_Declare(
  sqlite3_src
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
)

FetchContent_MakeAvailable(ftxui nlohmann_json sqlite3_src)

add_library(sqlite3 STATIC
  ${sqlite3_src_SOURCE_DIR}/sqlite3.c
)
target_include_directories(sqlite3 PUBLIC ${sqlite3_src_SOURCE_DIR})
target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=1 SQLITE_ENABLE_FTS5 SQLITE_ENABLE_JSON1)

set(LIBRAFLOW_SOURCES
  src/main.cpp
  src/utils/Config.cpp
  src/utils/Logger.cpp
  src/utils/Time.cpp
  src/utils/Hashing.cpp
  src/utils/CsvExporter.cpp
  src/db/Database.cpp
  src/db/Migrations.cpp
  src/db/Repositories.cpp
  src/app/Services.cpp
  src/ui/TerminalApp.cpp
)

add_executable(LibraFlow ${LIBRAFLOW_SOURCES})
target_include_directories(LibraFlow PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${sqlite3_src_SOURCE_DIR}
)
target_link_libraries(LibraFlow PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
  nlohmann_json::nlohmann_json
  sqlite3
)

if(MSVC)
  target_compile_options(LibraFlow PRIVATE /W4 /EHsc)
else()
  target_compile_options(LibraFlow PRIVATE -Wall -Wextra -Wpedantic)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_TESTS)
  add_executable(LibraFlowTests
    tests/test_runner.cpp
    src/utils/Time.cpp
    src/utils/Hashing.cpp
  )
  target_include_directories(LibraFlowTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  if(MSVC)
    target_compile_options(LibraFlowTests PRIVATE /W4 /EHsc)
  else()
    target_compile_options(LibraFlowTests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()
