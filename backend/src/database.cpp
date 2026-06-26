#include "database.hpp"
#include <stdexcept>
#include <iostream>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + path);
    }
    exec(R"(
        CREATE TABLE IF NOT EXISTS history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            query TEXT NOT NULL,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
    )");
    exec(R"(
        CREATE TABLE IF NOT EXISTS bookmarks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            url TEXT NOT NULL,
            snippet TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        );
    )");
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown sqlite error";
        sqlite3_free(err);
        throw std::runtime_error("SQLite error: " + msg);
    }
}

void Database::add_history(const std::string& query) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO history (query) VALUES (?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare insert history statement");
    }
    sqlite3_bind_text(stmt, 1, query.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<HistoryEntry> Database::get_history(int limit) {
    std::vector<HistoryEntry> out;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, query, created_at FROM history ORDER BY id DESC LIMIT ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare select history statement");
    }
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryEntry h;
        h.id = sqlite3_column_int(stmt, 0);
        h.query = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        h.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        out.push_back(h);
    }
    sqlite3_finalize(stmt);
    return out;
}

int Database::add_bookmark(const std::string& title, const std::string& url, const std::string& snippet) {
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO bookmarks (title, url, snippet) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare insert bookmark statement");
    }
    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, snippet.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

std::vector<BookmarkEntry> Database::get_bookmarks() {
    std::vector<BookmarkEntry> out;
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id, title, url, snippet, created_at FROM bookmarks ORDER BY id DESC;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare select bookmarks statement");
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BookmarkEntry b;
        b.id = sqlite3_column_int(stmt, 0);
        b.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        b.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const unsigned char* snip = sqlite3_column_text(stmt, 3);
        b.snippet = snip ? reinterpret_cast<const char*>(snip) : "";
        b.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        out.push_back(b);
    }
    sqlite3_finalize(stmt);
    return out;
}

void Database::remove_bookmark(int id) {
    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM bookmarks WHERE id = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare delete bookmark statement");
    }
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
