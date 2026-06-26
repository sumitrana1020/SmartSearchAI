#include "ai_service.hpp"
#include "http_client.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

namespace {
std::string env_or(const char* key, const std::string& fallback) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : fallback;
}
}

AIService::AIService() {
    api_key_ = env_or("LLM_API_KEY", "");
    model_ = env_or("LLM_MODEL", "claude-sonnet-4-6");
    endpoint_ = env_or("LLM_API_ENDPOINT", "https://api.anthropic.com/v1/messages");
}

std::string AIService::build_prompt(const std::string& query,
                                     const std::vector<SearchResult>& sources) {
    std::ostringstream oss;
    oss << "You are SmartSearch AI, an assistant that answers questions using ONLY "
        << "the numbered sources provided below. Write a concise, well-structured "
        << "answer (2-4 short paragraphs max). After every factual claim, add an "
        << "inline citation marker in the form [N] referencing the source number "
        << "it came from. If sources disagree or are insufficient, say so plainly. "
        << "Do not invent facts not present in the sources.\n\n";
    oss << "Question: " << query << "\n\nSources:\n";
    int i = 1;
    for (const auto& s : sources) {
        oss << i << ". " << s.title << " (" << s.domain << ")\n"
            << "   " << s.snippet << "\n";
        i++;
    }
    oss << "\nWrite the answer now, using [N] citation markers inline.";
    return oss.str();
}

AISummary AIService::summarize(const std::string& query,
                                const std::vector<SearchResult>& sources) {
    if (api_key_.empty()) {
        throw std::runtime_error("LLM_API_KEY is not set. See .env.example.");
    }

    auto start = std::chrono::steady_clock::now();

    json body = {
        {"model", model_},
        {"max_tokens", 600},
        {"messages", json::array({
            { {"role", "user"}, {"content", build_prompt(query, sources)} }
        })}
    };

    std::map<std::string, std::string> headers = {
        {"x-api-key", api_key_},
        {"anthropic-version", "2023-06-01"}
    };

    auto res = http::post_json(endpoint_, body.dump(), headers);
    if (!res.ok()) {
        throw std::runtime_error("LLM API error (" + std::to_string(res.status_code) + "): " + res.body);
    }

    json parsed = json::parse(res.body, nullptr, false);
    if (parsed.is_discarded()) {
        throw std::runtime_error("LLM API returned invalid JSON");
    }

    std::string text;
    if (parsed.contains("content") && parsed["content"].is_array()) {
        for (const auto& block : parsed["content"]) {
            if (block.value("type", "") == "text") {
                text += block.value("text", "");
            }
        }
    }

    auto end = std::chrono::steady_clock::now();

    AISummary summary;
    summary.answer_html = text;
    summary.model_used = model_;
    summary.latency_seconds = std::chrono::duration<double>(end - start).count();
    return summary;
}
