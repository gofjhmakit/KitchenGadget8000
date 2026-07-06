#include "core/Storage.h"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_spiffs.h"

namespace core {
namespace {
constexpr const char* TAG = "Storage";
}

Storage& Storage::instance() {
    static Storage inst;
    return inst;
}

std::string Storage::absolute_path(const std::string& path) const {
    if (path.rfind(base_path_, 0) == 0) {
        return path;
    }
    if (!path.empty() && path.front() == '/') {
        return base_path_ + path;
    }
    return base_path_ + "/" + path;
}

bool Storage::init(const char* base_path) {
    base_path_ = base_path;
    esp_vfs_spiffs_conf_t conf{};
    conf.base_path = base_path;
    conf.partition_label = nullptr;
    conf.max_files = 16;
    conf.format_if_mount_failed = true;
    const esp_err_t err = esp_vfs_spiffs_register(&conf);
    mounted_ = (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
    if (!mounted_) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return false;
    }
    ::mkdir((base_path_ + "/recipes").c_str(), 0755);
    return true;
}

bool Storage::write_file(const std::string& path, const std::string& data) {
    std::ofstream file(absolute_path(path), std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    file << data;
    return file.good();
}

bool Storage::read_file(const std::string& path, std::string& data) {
    std::ifstream file(absolute_path(path), std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    data = ss.str();
    return true;
}

bool Storage::file_exists(const std::string& path) {
    struct stat st{};
    return ::stat(absolute_path(path).c_str(), &st) == 0;
}

bool Storage::delete_file(const std::string& path) {
    return ::unlink(absolute_path(path).c_str()) == 0;
}

std::vector<std::string> Storage::list_files(const std::string& dir, const std::string& ext) {
    std::vector<std::string> results;
    const std::string abs = absolute_path(dir);
    DIR* dp = opendir(abs.c_str());
    if (dp == nullptr) {
        return results;
    }
    while (dirent* ent = readdir(dp)) {
        if (ent->d_type != DT_REG) {
            continue;
        }
        std::string name = ent->d_name;
        if (!ext.empty() && (name.size() < ext.size() || name.substr(name.size() - ext.size()) != ext)) {
            continue;
        }
        results.push_back(name);
    }
    closedir(dp);
    std::sort(results.begin(), results.end());
    return results;
}

size_t Storage::free_bytes() const {
    size_t total = 0, used = 0;
    if (esp_spiffs_info(nullptr, &total, &used) != ESP_OK || total == 0) {
        return 0;
    }
    return total - used;
}

} // namespace core
