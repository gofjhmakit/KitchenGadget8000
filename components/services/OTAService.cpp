#include "services/OTAService.h"

#include "esp_https_ota.h"
#include "esp_log.h"
#include "core/Network.h"

namespace services {
namespace {
std::string field_string(const std::string& json, const char* key) {
    const std::string token = std::string("\"") + key + "\":\"";
    auto pos = json.find(token);
    if (pos == std::string::npos) return {};
    pos += token.size();
    auto end = json.find('"', pos);
    return json.substr(pos, end - pos);
}
}

OTAService& OTAService::instance() {
    static OTAService inst;
    return inst;
}

bool OTAService::check_for_update(const std::string& repo, std::string& version, std::string& url) {
    std::string body;
    if (!core::Network::instance().http_get("https://api.github.com/repos/" + repo + "/releases/latest", body, {{"Accept", "application/vnd.github+json"}})) return false;
    version = field_string(body, "tag_name");
    const auto asset = body.find("browser_download_url");
    if (asset != std::string::npos) {
        auto start = body.find("https://", asset);
        auto end = body.find('"', start);
        url = body.substr(start, end - start);
    }
    return !version.empty() && !url.empty();
}

bool OTAService::apply_update(const std::string& url, ProgressCallback progress_cb) {
    if (progress_cb) progress_cb(0);
    esp_http_client_config_t http_cfg{};
    http_cfg.url = url.c_str();
    esp_https_ota_config_t ota_cfg{};
    ota_cfg.http_config = &http_cfg;
    const esp_err_t err = esp_https_ota(&ota_cfg);
    if (progress_cb) progress_cb(err == ESP_OK ? 100 : -1);
    return err == ESP_OK;
}

} // namespace services
