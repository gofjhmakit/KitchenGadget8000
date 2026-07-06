#pragma once
#include <functional>
#include <string>
#include <vector>

namespace core {

class Storage {
public:
    static Storage& instance();
    bool init(const char* base_path = "/spiffs");
    bool write_file(const std::string& path, const std::string& data);
    bool read_file(const std::string& path, std::string& data);
    bool file_exists(const std::string& path);
    bool delete_file(const std::string& path);
    std::vector<std::string> list_files(const std::string& dir, const std::string& ext = "");
    std::string base_path() const { return base_path_; }
    size_t free_bytes() const;

private:
    Storage() = default;
    std::string absolute_path(const std::string& path) const;
    std::string base_path_{"/spiffs"};
    bool mounted_{false};
};

} // namespace core
