#include "jukebox/download/hosted.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <Geode/Result.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/general.hpp>
#include <arc/future/Future.hpp>
#include <arc/time/Sleep.hpp>
#include <fmt/core.h>

#include "jukebox/events/file_download_progress.hpp"

using namespace geode::prelude;

namespace jukebox::download {

static std::string makeTempPath() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::string hex;
    for (int i = 0; i < 16; i++) {
        hex += "0123456789abcdef"[dis(gen)];
    }
    return (std::filesystem::temp_directory_path() / (hex + ".bin")).string();
}

static Result<ByteVector> downloadWithCurl(const std::string& url, int timeout) {
    const std::string tempFile = makeTempPath();

    std::string cmd = "curl -sL --max-time " + std::to_string(timeout) +
                      " -o \"" + tempFile + "\" \"" + url + "\"";

    int code = std::system(cmd.c_str());
    if (code != 0) {
        std::error_code ec;
        std::filesystem::remove(tempFile, ec);
        return Err("curl failed with exit code {}", code);
    }

    std::ifstream file(tempFile, std::ios::binary);
    if (!file) {
        std::error_code ec;
        std::filesystem::remove(tempFile, ec);
        return Err("Failed to read downloaded file");
    }

    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    ByteVector data;
    if (size > 0) {
        data.resize(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), size);
    }
    file.close();

    std::error_code ec;
    std::filesystem::remove(tempFile, ec);

    if (data.empty()) { return Err("Downloaded file is empty"); }

    return Ok(std::move(data));
}

arc::Future<Result<ByteVector>> startHostedDownload(const std::string_view url) {
    int timeout = Mod::get()->getSettingValue<int>("download-timeout");
    timeout = std::max(timeout, 30);

    std::promise<Result<ByteVector>> prom;
    auto fut = prom.get_future();

    std::thread worker([&prom, url = std::string(url), timeout]() {
        prom.set_value(downloadWithCurl(url, timeout));
    });

    while (fut.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready) {
        co_await arc::sleep(asp::time::Duration::fromMillis(50));
    }

    worker.join();
    co_return fut.get();
}

}  // namespace jukebox::download
