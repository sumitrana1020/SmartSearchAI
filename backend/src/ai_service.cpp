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
    model_ = env_or("LLM_MODEL", "gemini-2.5-flash");
    // Gemini's endpoint is built from the model name; LLM_API_ENDPOINT lets
    // you override the base URL if needed (e.g. a different API version).
    endpoint_ = env_or("LLM_API_ENDPOINT",
        "https://generativelanguage.googleapis.com/v1beta/models/" + model_ + ":generateContent");
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

    // Gemini expects: { "contents": [ { "parts": [ { "text": "..." } ] } ] }
    json body = {
        {"contents", json::array({
            { {"parts", json::array({ { {"text", build_prompt(query, sources)} } })} }
        })},
        {"generationConfig", { {"maxOutputTokens", 600} }}
    };

    // Gemini authenticates via a "key" query parameter rather than a header.
    std::string url = endpoint_ + "?key=" + api_key_;

    std::map<std::string, std::string> headers = {};

    auto res = http::post_json(url, body.dump(), headers);
    if (!res.ok()) {
        throw std::runtime_error("LLM API error (" + std::to_string(res.status_code) + "): " + res.body);
    }

    json parsed = json::parse(res.body, nullptr, false);
    if (parsed.is_discarded()) {
        throw std::runtime_error("LLM API returned invalid JSON");
    }

    std::string text;
    if (parsed.contains("candidates") && parsed["candidates"].is_array() && !parsed["candidates"].empty()) {
        const auto& content = parsed["candidates"][0]["content"];
        if (content.contains("parts") && content["parts"].is_array()) {
            for (const auto& part : content["parts"]) {
                text += part.value("text", "");
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
