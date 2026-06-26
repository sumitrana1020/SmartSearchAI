#include "crow.h"
#include "crow/middlewares/cors.h"
#include <nlohmann/json.hpp>
#include "search_service.hpp"
#include "ai_service.hpp"
#include "database.hpp"
#include <regex>

using json = nlohmann::json;

// Converts [N] markers in plain text into <cite data-src="N">N</cite> markers
// the frontend already knows how to render and wire up to the source rail.
static std::string convert_citation_markers(const std::string& text) {
    std::regex marker_re(R"(\[(\d+(?:,\d+)*)\])");
    return std::regex_replace(text, marker_re,
        R"(<span class="cite" data-src="$1">$1</span>)");
}

int main() {
    crow::App<crow::CORSHandler> app;

    // Allow the frontend (served from anywhere during dev) to call this API.
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("*")
        .methods("GET"_method, "POST"_method, "DELETE"_method, "OPTIONS"_method)
        .headers("Content-Type");

    SearchService search_service;
    AIService ai_service;
    Database db("smartsearch.db");

    // Static trending list. In production this would be computed from
    // aggregated query logs; kept simple here.
    std::vector<std::string> trending = {
        "AI search engines 2026",
        "Crow framework C++",
        "Vector databases",
        "LLM citation accuracy"
    };

    // ---------------- POST /api/search ----------------
    CROW_ROUTE(app, "/api/search").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("query")) {
            return crow::response(400, R"({"error":"Missing 'query' field"})");
        }

        std::string query = body.value("query", "");
        std::string lang = body.value("lang", "en");

        try {
            auto results = search_service.search(query, lang, 6);

            json sources_json = json::array();
            for (size_t i = 0; i < results.size(); ++i) {
                sources_json.push_back({
                    {"num", i + 1},
                    {"title", results[i].title},
                    {"url", results[i].url},
                    {"domain", results[i].domain},
                    {"snippet", results[i].snippet}
                });
            }

            json response_json;
            response_json["sources"] = sources_json;

            if (!results.empty()) {
                auto summary = ai_service.summarize(query, results);
                response_json["answer_html"] = convert_citation_markers(summary.answer_html);
                response_json["model"] = summary.model_used;
                response_json["latency_seconds"] = summary.latency_seconds;
            } else {
                response_json["answer_html"] = "No results were found for this query.";
                response_json["model"] = nullptr;
                response_json["latency_seconds"] = 0;
            }

            db.add_history(query);

            crow::response res(response_json.dump());
            res.set_header("Content-Type", "application/json");
            return res;

        } catch (const std::exception& e) {
            json err = {{"error", e.what()}};
            crow::response res(502, err.dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
    });

    // ---------------- GET /api/autocomplete?q= ----------------
    CROW_ROUTE(app, "/api/autocomplete")
    ([&](const crow::request& req) {
        auto q = req.url_params.get("q");
        std::string query = q ? std::string(q) : "";
        auto suggestions = search_service.autocomplete(query);

        json arr = json::array();
        for (auto& s : suggestions) arr.push_back(s);

        crow::response res(json{{"suggestions", arr}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- GET /api/trending ----------------
    CROW_ROUTE(app, "/api/trending")
    ([&]() {
        json arr = json::array();
        for (auto& t : trending) arr.push_back(t);
        crow::response res(json{{"trending", arr}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- GET /api/history ----------------
    CROW_ROUTE(app, "/api/history")
    ([&]() {
        auto entries = db.get_history(20);
        json arr = json::array();
        for (auto& h : entries) {
            arr.push_back({
                {"id", h.id},
                {"query", h.query},
                {"created_at", h.created_at}
            });
        }
        crow::response res(json{{"history", arr}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- GET /api/bookmarks ----------------
    CROW_ROUTE(app, "/api/bookmarks")
    ([&]() {
        auto entries = db.get_bookmarks();
        json arr = json::array();
        for (auto& b : entries) {
            arr.push_back({
                {"id", b.id},
                {"title", b.title},
                {"url", b.url},
                {"snippet", b.snippet},
                {"created_at", b.created_at}
            });
        }
        crow::response res(json{{"bookmarks", arr}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- POST /api/bookmarks ----------------
    CROW_ROUTE(app, "/api/bookmarks").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("title") || !body.contains("url")) {
            return crow::response(400, R"({"error":"Missing 'title' or 'url'"})");
        }
        int id = db.add_bookmark(
            body.value("title", ""),
            body.value("url", ""),
            body.value("snippet", "")
        );
        crow::response res(json{{"id", id}, {"status", "saved"}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- DELETE /api/bookmarks/<id> ----------------
    CROW_ROUTE(app, "/api/bookmarks/<int>").methods("DELETE"_method)
    ([&](int id) {
        db.remove_bookmark(id);
        crow::response res(json{{"status", "deleted"}}.dump());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    // ---------------- GET /api/health ----------------
    CROW_ROUTE(app, "/api/health")
    ([]() {
        return crow::response(json{{"status", "ok"}}.dump());
    });

    int port = 8080;
    if (const char* p = std::getenv("PORT")) port = std::atoi(p);

    std::cout << "SmartSearch AI backend running on http://localhost:" << port << "\n";
    app.port(port).multithreaded().run();
    return 0;
}
