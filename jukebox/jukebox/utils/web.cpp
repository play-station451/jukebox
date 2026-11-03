#include "jukebox/utils/web.hpp"

#include <cstdlib>
#include <optional>
#include <string>

#include <Geode/utils/web.hpp>
#include <fmt/format.h>

namespace jukebox::utils::web {

std::string getErrorFromResponse(const geode::utils::web::WebResponse& res) {
    if (res.ok()) { return ""; }

    // Code < 0 => cURL / internal Geode error - very bad!
    if (res.code() < 0) {
        if (!res.errorMessage().empty()) { return std::string(res.errorMessage()); }

        return res.string().unwrapOr(fmt::format("Failed to send request: internal code {}", res.code()));
    }

    if (res.code() == 502 || res.code() == 503 || res.code() == 530) {
        return "Web request failed. Service is currently "
               "unavailable or under maintenance. Please try "
               "again later.";
    }

    if (res.badServer()) {
        return fmt::format("Web request failed because of a server error, status code {}", res.code());
    }

    return fmt::format("Web request failed with status code {}", res.code());
}

std::optional<geode::utils::web::ProxyOpts> getProxyOptsFromEnv() {
    const char* vars[] = {"https_proxy", "HTTPS_PROXY", "http_proxy", "HTTP_PROXY", "all_proxy", "ALL_PROXY"};
    std::string url;
    for (const char* v : vars) {
        if (const char* e = std::getenv(v)) {
            if (*e != '\0') { url = e; break; }
        }
    }
    if (url.empty()) { return std::nullopt; }

    geode::utils::web::ProxyOpts opts;
    if (url.rfind("socks5h://", 0) == 0) { opts.type = geode::utils::web::ProxyType::SOCKS5H; }
    else if (url.rfind("socks5://", 0) == 0) { opts.type = geode::utils::web::ProxyType::SOCKS5; }
    else if (url.rfind("socks4a://", 0) == 0) { opts.type = geode::utils::web::ProxyType::SOCKS4A; }
    else if (url.rfind("socks4://", 0) == 0) { opts.type = geode::utils::web::ProxyType::SOCKS4; }
    else if (url.rfind("https://", 0) == 0) { opts.type = geode::utils::web::ProxyType::HTTPS; }
    else { opts.type = geode::utils::web::ProxyType::HTTP; }

    // Geode's proxyOpts expects a host[:port] address (the proxy type is set
    // separately), so strip any scheme/path from the URL.
    std::string addr = url;
    const auto schemePos = addr.find("://");
    if (schemePos != std::string::npos) {
        addr = addr.substr(schemePos + 3);
    }
    const auto slashPos = addr.find('/');
    if (slashPos != std::string::npos) {
        addr = addr.substr(0, slashPos);
    }
    opts.address = addr;
    opts.tunneling = true;
    return opts;
}

}  // namespace jukebox::utils::web
