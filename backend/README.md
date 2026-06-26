# SmartSearch AI — Backend (C++ / Crow)

REST API backend for SmartSearch AI. Fetches live web search results and asks
an LLM to produce a short, cited summary, plus endpoints for history,
trending, and bookmarks (SQLite-backed).

## Endpoints

| Method | Path                  | Description                              |
|--------|-----------------------|-------------------------------------------|
| POST   | `/api/search`         | `{ "query": "...", "lang": "en" }` → AI summary + sources |
| GET    | `/api/autocomplete?q=`| Query suggestions                          |
| GET    | `/api/trending`       | Trending search topics                     |
| GET    | `/api/history`        | Recent search history                      |
| GET    | `/api/bookmarks`      | Saved bookmarks                            |
| POST   | `/api/bookmarks`      | `{ "title", "url", "snippet" }` → save one |
| DELETE | `/api/bookmarks/<id>` | Remove a bookmark                          |
| GET    | `/api/health`         | Health check                               |

## 1. Install system dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y build-essential cmake libcurl4-openssl-dev libsqlite3-dev
```

**macOS (Homebrew):**
```bash
brew install cmake curl sqlite3
```

Crow and nlohmann/json are fetched automatically by CMake (`FetchContent`) —
no manual install needed for those, but your machine needs internet access
on first build.

## 2. Set your API keys

Copy the example env file:
```bash
cp .env.example .env
```

Then load it into your shell before running the server:
```bash
export $(grep -v '^#' .env | xargs)
```

You need:
- A **search API key** — [serper.dev](https://serper.dev) has a free tier and is the simplest to start with (Google results as JSON). You can swap `SEARCH_API_ENDPOINT` for Bing/Brave later — see `search_service.cpp`.
- An **LLM API key** — from [console.anthropic.com](https://console.anthropic.com) (or swap `ai_service.cpp` for OpenAI's API if you prefer).

## 3. Build

```bash
mkdir build && cd build
cmake ..
cmake --build . -j4
```

## 4. Run

```bash
./smartsearch_backend
```

You should see:
```
SmartSearch AI backend running on http://localhost:8080
```

## 5. Test it

```bash
curl -X POST http://localhost:8080/api/search \
  -H "Content-Type: application/json" \
  -d '{"query": "what does idempotent mean"}'
```

## Connecting the frontend

The frontend currently uses static demo data. To wire it up for real:
in `smartsearch-ai.html`'s `<script>` section, replace the static answer/source
HTML with a `fetch('http://localhost:8080/api/search', { method: 'POST', ... })`
call on button click, and render `answer_html` / `sources` into the existing
`.answer-text` and `.source-list` containers. Ask me when you're ready and
I'll write that wiring for you.

## Notes

- `smartsearch.db` (SQLite file) is created automatically on first run in the
  working directory — don't commit it to git (already in `.gitignore`).
- CORS is wide open (`*`) for local development — tighten this before deploying.
- Swap search/LLM providers by changing only `search_service.cpp` /
  `ai_service.cpp` — the rest of the app doesn't need to change.
