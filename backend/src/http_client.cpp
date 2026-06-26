#include "http_client.hpp"
#include <curl/curl.h>
#include <stdexcept>

namespace http {

namespace {
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* out = static_cast<std::string*>(userp);
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

curl_slist* build_headers(const std::map<std::string, std::string>& headers) {
    curl_slist* list = nullptr;
    for (const auto& [key, value] : headers) {
        std::string line = key + ": " + value;
        list = curl_slist_append(list, line.c_str());
    }
    return list;
}
} // namespace

Response post_json(const std::string& url,
                    const std::string& json_body,
                    const std::map<std::string, std::string>& headers) {
    Response res;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    std::map<std::string, std::string> all_headers = headers;
    all_headers["Content-Type"] = "application/json";
    curl_slist* hlist = build_headers(all_headers);

    std::string buffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        res.status_code = code;
        res.body = buffer;
    } else {
        res.status_code = 0;
        res.body = curl_easy_strerror(rc);
    }

    curl_slist_free_all(hlist);
    curl_easy_cleanup(curl);
    return res;
}

Response get(const std::string& url, const std::map<std::string, std::string>& headers) {
    Response res;
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    curl_slist* hlist = build_headers(headers);
    std::string buffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        res.status_code = code;
        res.body = buffer;
    } else {
        res.status_code = 0;
        res.body = curl_easy_strerror(rc);
    }

    curl_slist_free_all(hlist);
    curl_easy_cleanup(curl);
    return res;
}

std::string url_encode(const std::string& value) {
    CURL* curl = curl_easy_init();
    char* encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

} // namespace http
