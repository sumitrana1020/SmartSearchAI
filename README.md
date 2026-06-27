# SmartSearch AI

**Cited answers, not just links.**

SmartSearch AI is a full-stack AI-powered search engine. Instead of returning a list of blue links, it fetches real-time web results, asks an LLM to summarize them, and attaches numbered citation markers to every claim — so you can verify exactly where each piece of the answer came from.

The whole experience is built around one idea: **transparency**. Click any citation marker in the AI summary and it highlights and scrolls to the exact source card it came from, like flipping to a footnote in a printed book.

---

## ✨ Features

- 🔎 Real-time web search (via [Serper.dev](https://serper.dev))
- 🤖 AI-generated answer summaries with inline citation markers (via Google Gemini)
- 📎 Clickable citations that link directly to their source card
- 💬 Natural language question answering
- ⌨️ Intelligent autocomplete suggestions
- 🕘 Search history (SQLite-backed)
- 🔥 Trending searches
- ⭐ Bookmark / favorite results
- 🌗 Dark and light mode
- 📱 Fully responsive layout
- 🌍 Multi-language search (UI in place, backend wiring in progress)
- 🎙️ Voice search — *planned*

---

## 🏗️ Architecture

```
┌──────────────────┐        REST/JSON        ┌──────────────────────┐
│   Frontend        │ ───────────────────────▶│   Backend (C++ /     │
│   HTML/CSS/JS      │ ◀───────────────────────│   Crow framework)    │
└──────────────────┘                           └──────────┬────────────┘
                                                            │
                          ┌─────────────────────────────────┼─────────────────────────────────┐
                          ▼                                 ▼                                 ▼
                  ┌───────────────┐                ┌────────────────┐                ┌─────────────────┐
                  │ Serper.dev API │                │ Google Gemini  │                │ SQLite          │
                  │ (web search)   │                │ API (summary)  │                │ (history/marks) │
                  └───────────────┘                └────────────────┘                └─────────────────┘
```

**Flow:** user query → backend fetches live search results → results + query sent to Gemini with a prompt that forces `[N]` citation markers → markers converted to clickable spans → response (answer + sources) returned as JSON → frontend renders it and saves the query to history.

---

## 📁 Project structure

```
SmartSearchAI/
├── frontend/
│   └── smartsearch-ai.html      # Single-file UI (HTML/CSS/JS), no build step needed
└── backend/
    ├── CMakeLists.txt           # Fetches Crow + nlohmann/json automatically
    ├── README.md                # Backend-specific setup details
    ├── .env.example             # Template for API keys (copy to .env)
    ├── include/
    │   ├── http_client.hpp
    │   ├── search_service.hpp
    │   ├── ai_service.hpp
    │   └── database.hpp
    └── src/
        ├── http_client.cpp      # libcurl wrapper used by both services below
        ├── search_service.cpp   # Calls Serper.dev for live web results
        ├── ai_service.cpp       # Calls Gemini, builds the cited summary
        ├── database.cpp         # SQLite: history + bookmarks
        └── main.cpp             # Crow REST routes
```

---

## 🛠️ Tech stack

| Layer | Technology |
|---|---|
| Backend | C++17, [Crow](https://crowcpp.org) web framework |
| Frontend | HTML5, CSS3, vanilla JavaScript |
| Database | SQLite |
| Web search | Serper.dev (Google results as JSON) |
| AI summarization | Google Gemini API (free tier) |
| HTTP client | libcurl |
| JSON | nlohmann/json |
| Build system | CMake |

---

## 🚀 Getting started

### 1. Backend

```bash
cd backend
cp .env.example .env
# edit .env and add your real Serper + Gemini API keys
export $(grep -v '^#' .env | xargs)

mkdir build && cd build
cmake .. -G "MinGW Makefiles"     # drop the -G flag on Linux/Mac
cmake --build . -j4

./smartsearch_backend.exe          # or ./smartsearch_backend on Linux/Mac
```

Server runs at `http://localhost:8080`. Full backend setup details (dependencies, MSYS2/Windows notes, troubleshooting) live in [`backend/README.md`](backend/README.md).

### 2. Frontend

With the backend running, just open `frontend/smartsearch-ai.html` directly in your browser — no build step required. Type a question and hit **ASK →**.

---

## 🔑 Getting API keys (both have free tiers, no credit card needed)

- **Search:** [serper.dev](https://serper.dev) — 2,500 free queries to start
- **AI summarization:** [aistudio.google.com/apikey](https://aistudio.google.com/apikey) — Google Gemini's free tier, no card required

---

## 📡 API reference

| Method | Endpoint | Description |
|---|---|---|
| `POST` | `/api/search` | `{ "query": "...", "lang": "en" }` → AI summary + cited sources |
| `GET` | `/api/autocomplete?q=` | Query suggestions |
| `GET` | `/api/trending` | Trending search topics |
| `GET` | `/api/history` | Recent search history |
| `GET` | `/api/bookmarks` | Saved bookmarks |
| `POST` | `/api/bookmarks` | `{ "title", "url", "snippet" }` → save one |
| `DELETE` | `/api/bookmarks/<id>` | Remove a bookmark |
| `GET` | `/api/health` | Health check |

---

## 🗺️ Roadmap

- [x] Live web search integration
- [x] AI-generated cited summaries
- [x] Search history & trending
- [x] Dark/light mode
- [ ] Real bookmark persistence wired to the frontend
- [ ] Multi-language search end-to-end
- [ ] Voice search

---

## 📄 License

This is a personal learning/portfolio project. Feel free to fork and build on it.