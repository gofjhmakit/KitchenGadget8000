#include "services/GitHubSync.h"

#include <sstream>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/Network.h"
#include "core/Storage.h"
#include "core/Settings.h"
#include "esp_log.h"

namespace services {
namespace {
constexpr const char* TAG = "GitHubSync";

struct RemoteFile { std::string name; std::string sha; std::string download_url; };

std::vector<RemoteFile> parse_listing(const std::string& json) {
    std::vector<RemoteFile> files;
    size_t pos = 0;
    while ((pos = json.find("\"name\":\"", pos)) != std::string::npos) {
        pos += 8;
        auto end = json.find('"', pos);
        RemoteFile file; file.name = json.substr(pos, end - pos);
        auto sha_pos = json.find("\"sha\":\"", end); if (sha_pos != std::string::npos) { sha_pos += 7; auto sha_end = json.find('"', sha_pos); file.sha = json.substr(sha_pos, sha_end - sha_pos); }
        auto dl_pos = json.find("\"download_url\":\"", end); if (dl_pos != std::string::npos) { dl_pos += 16; auto dl_end = json.find('"', dl_pos); file.download_url = json.substr(dl_pos, dl_end - dl_pos); pos = dl_end; }
        files.push_back(file);
    }
    return files;
}

void sync_task(void* arg) {
    auto* self = static_cast<GitHubSync*>(arg);
    while (self->initialized_) {
        self->sync_once();
        vTaskDelay(pdMS_TO_TICKS(self->interval_sec_ * 1000ULL));
    }
    vTaskDelete(nullptr);
}
}

GitHubSync& GitHubSync::instance() {
    static GitHubSync inst;
    return inst;
}

void GitHubSync::init(const char* repo, const char* branch) {
    repo_ = repo ? repo : "";
    branch_ = branch ? branch : "main";
    interval_sec_ = core::Settings::instance().get().github_sync_interval_sec;
    initialized_ = true;
    xTaskCreate(sync_task, "github_sync", 6144, this, 2, nullptr);
}

void GitHubSync::set_interval(uint32_t seconds) { interval_sec_ = seconds; }

bool GitHubSync::sync_once() {
    if (!core::Network::instance().is_connected() || repo_.empty()) return false;
    std::string listing;
    const std::string api = "https://api.github.com/repos/" + repo_ + "/contents/recipes?ref=" + branch_;
    if (!core::Network::instance().http_get(api, listing, {{"Accept", "application/vnd.github+json"}})) {
        ESP_LOGW(TAG, "Recipe listing fetch failed");
        return false;
    }
    core::Storage::instance().write_file("/recipes/.keep", "");
    for (const auto& file : parse_listing(listing)) {
        const std::string etag_path = "/recipes/." + file.name + ".etag";
        std::string existing_sha;
        core::Storage::instance().read_file(etag_path, existing_sha);
        if (existing_sha == file.sha) continue;
        std::string content;
        if (!core::Network::instance().http_get(file.download_url, content)) continue;
        core::Storage::instance().write_file("/recipes/" + file.name, content);
        core::Storage::instance().write_file(etag_path, file.sha);
    }
    return true;
}

} // namespace services
