#pragma once
#include <string>
#include <vector>
#include "search_service.hpp"

struct AISummary {
    std::string answer_html;     // answer text with <cite data-src="N"> markers
    std::string model_used;
    double latency_seconds = 0.0;
};

class AIService {
public:
    AIService();

    // Sends the query + retrieved sources to the configured LLM and asks
    // it to produce a short, cited summary. Throws std::runtime_error on
    // network/API failure.
    AISummary summarize(const std::string& query,
                         const std::vector<SearchResult>& sources);

private:
    std::string api_key_;
    std::string model_;
    std::string endpoint_;

    std::string build_prompt(const std::string& query,
                              const std::vector<SearchResult>& sources);
};
