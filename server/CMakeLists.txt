cmake_minimum_required(VERSION 3.16)
project(budget_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
  else()
    add_compile_options(-g -fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
endif()

# ── Dependencies via FetchContent ──────────────────────────────────────────────
include(FetchContent)

# cpp-httplib (header-only HTTP server/client)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.15.3
  GIT_SHALLOW    TRUE
)

# nlohmann/json (header-only JSON library)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)

# Catch2 (testing framework)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(httplib nlohmann_json Catch2)

# ── System dependencies ────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# ── Main server executable ─────────────────────────────────────────────────────
set(SERVER_SOURCES
  src/main.cpp
  src/database.cpp
  src/auth.cpp
  src/handlers/auth_handler.cpp
  src/handlers/transaction_handler.cpp
  src/handlers/other_handlers.cpp
)

add_executable(budget_server ${SERVER_SOURCES})

target_include_directories(budget_server PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(budget_server PRIVATE
  httplib::httplib
  nlohmann_json::nlohmann_json
  OpenSSL::SSL
  OpenSSL::Crypto
  SQLite::SQLite3
  Threads::Threads
)

# Enable HTTPS support in cpp-httplib
target_compile_definitions(budget_server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()

set(TEST_SOURCES
  tests/test_main.cpp
  tests/test_auth.cpp
  tests/test_transactions.cpp
  tests/test_reports.cpp
  src/database.cpp
  src/auth.cpp
)

add_executable(budget_tests ${TEST_SOURCES})

target_include_directories(budget_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(budget_tests PRIVATE
  Catch2::Catch2WithMain
  nlohmann_json::nlohmann_json
  OpenSSL::SSL
  OpenSSL::Crypto
  SQLite::SQLite3
  Threads::Threads
)

include(CTest)
include(Catch)
catch_discover_tests(budget_tests)

# ── Install ────────────────────────────────────────────────────────────────────
install(TARGETS budget_server RUNTIME DESTINATION bin)
install(FILES config.json DESTINATION etc/budget_server)
