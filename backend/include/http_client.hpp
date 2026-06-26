#pragma once
#include <string>
#include <map>

// Minimal libcurl-based HTTP client used by SearchService and AIService.
namespace http {

struct Response {
    long status_code = 0;
    std::string body;
    bool ok() const { return status_code >= 200 && status_code < 300; }
};

// Performs a POST request with a JSON body and the given headers.
Response post_json(const std::string& url,
                    const std::string& json_body,
                    const std::map<std::string, std::string>& headers);

// Performs a GET request with the given headers.
Response get(const std::string& url,
              const std::map<std::string, std::string>& headers);

// URL-encodes a query parameter value.
std::string url_encode(const std::string& value);

} // namespace http
