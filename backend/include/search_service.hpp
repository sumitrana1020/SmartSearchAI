#pragma once
#include <string>
#include <vector>

struct SearchResult {
    std::string title;
    std::string url;
    std::string domain;
    std::string snippet;
};

class SearchService {
public:
    SearchService();

    // Calls the configured web search API (Serper.dev by default) and
    // returns up to `max_results` results for the given query.
    // Throws std::runtime_error on network/API failure.
    std::vector<SearchResult> search(const std::string& query,
                                      const std::string& lang = "en",
                                      int max_results = 6);

    // Lightweight autocomplete suggestions. For now this generates
    // suggestions locally; swap in a real suggest API later if needed.
    std::vector<std::string> autocomplete(const std::string& partial_query);

private:
    std::string api_key_;
    std::string endpoint_;
};
