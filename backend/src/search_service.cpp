#include "search_service.hpp"
#include "http_client.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

using json = nlohmann::json;

namespace {
std::string env_or(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : fallback;
}

std::string extract_domain(const std::string& url) {
    std::string s = url;
    auto pos = s.find("://");
    if (pos != std::string::npos) s = s.substr(pos + 3);
    auto slash = s.find('/');
    if (slash != std::string::npos) s = s.substr(0, slash);
    return s;
}
}

SearchService::SearchService() {
    api_key_ = env_or("SEARCH_API_KEY", "");
    endpoint_ = env_or("SEARCH_API_ENDPOINT", "https://google.serper.dev/search");
}

std::vector<SearchResult> SearchService::search(const std::string& query,
                                                 const std::string& lang,
                                                 int max_results) {
    if (api_key_.empty()) {
        throw std::runtime_error("SEARCH_API_KEY is not set. See .env.example.");
    }

    json body = {
        {"q", query},
        {"gl", "us"},
        {"hl", lang},
        {"num", max_results}
    };

    std::map<std::string, std::string> headers = {
        {"X-API-KEY", api_key_}
    };

    auto res = http::post_json(endpoint_, body.dump(), headers);
    if (!res.ok()) {
        throw std::runtime_error("Search API error (" + std::to_string(res.status_code) + "): " + res.body);
    }

    json parsed = json::parse(res.body, nullptr, false);
    if (parsed.is_discarded()) {
        throw std::runtime_error("Search API returned invalid JSON");
    }

    std::vector<SearchResult> results;
    if (parsed.contains("organic") && parsed["organic"].is_array()) {
        for (const auto& item : parsed["organic"]) {
            if (static_cast<int>(results.size()) >= max_results) break;
            SearchResult r;
            r.title = item.value("title", "");
            r.url = item.value("link", "");
            r.snippet = item.value("snippet", "");
            r.domain = extract_domain(r.url);
            if (!r.title.empty() && !r.url.empty()) {
                results.push_back(r);
            }
        }
    }
    return results;
}

std::vector<std::string> SearchService::autocomplete(const std::string& partial_query) {
    // Simple local suggestion generator. Replace with a real suggest API
    // (e.g. a search provider's autocomplete endpoint) when available.
    if (partial_query.empty()) return {};

    static const std::vector<std::string> templates = {
        "{q} meaning",
        "{q} explained",
        "how does {q} work",
        "{q} vs alternatives",
        "best {q} 2026"
    };

    std::vector<std::string> suggestions;
    for (const auto& t : templates) {
        std::string s = t;
        auto pos = s.find("{q}");
        s.replace(pos, 3, partial_query);
        suggestions.push_back(s);
    }
    return suggestions;
}
