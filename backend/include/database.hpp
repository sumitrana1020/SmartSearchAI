#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

struct HistoryEntry {
    int id;
    std::string query;
    std::string created_at;
};

struct BookmarkEntry {
    int id;
    std::string title;
    std::string url;
    std::string snippet;
    std::string created_at;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    void add_history(const std::string& query);
    std::vector<HistoryEntry> get_history(int limit = 20);

    int add_bookmark(const std::string& title, const std::string& url, const std::string& snippet);
    std::vector<BookmarkEntry> get_bookmarks();
    void remove_bookmark(int id);

private:
    sqlite3* db_ = nullptr;
    void exec(const std::string& sql);
};
