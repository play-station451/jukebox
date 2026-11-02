#pragma once

#include <optional>
#include <string>

#include <Geode/utils/web.hpp>

namespace jukebox::utils::web {

std::string getErrorFromResponse(const geode::utils::web::WebResponse& response);

// Reads proxy settings from the standard environment variables (http_proxy,
// https_proxy, all_proxy, ...) and returns them as ProxyOpts, if set.
// Returns std::nullopt when no proxy is configured.
std::optional<geode::utils::web::ProxyOpts> getProxyOptsFromEnv();

}  // namespace jukebox::utils::web
