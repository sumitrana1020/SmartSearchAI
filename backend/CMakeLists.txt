cmake_minimum_required(VERSION 3.16)
project(smartsearch_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- Crow (use master branch for newer Asio compatibility) ---
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG master
)
FetchContent_MakeAvailable(Crow)

# --- nlohmann/json ---
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# --- System deps ---
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(smartsearch_backend
  src/main.cpp
  src/search_service.cpp
  src/ai_service.cpp
  src/database.cpp
  src/http_client.cpp
)

target_include_directories(smartsearch_backend PRIVATE include)

target_link_libraries(smartsearch_backend
  PRIVATE
    Crow::Crow
    nlohmann_json::nlohmann_json
    CURL::libcurl
    SQLite::SQLite3
    Threads::Threads
)

# Windows-specific socket libraries
if(WIN32)
  target_link_libraries(smartsearch_backend PRIVATE ws2_32 mswsock)
endif()
