#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace services {

struct Recipe {
    std::string source_path;
    std::string title;
    std::string time;
    int servings{0};
    std::vector<std::string> tags;
    std::string image;       // emoji or short label from frontmatter
    std::string image_path;  // auto-derived SPIFFS path to .jpg (may not exist on device)
    std::vector<std::string> ingredients;
    std::vector<std::string> instructions;
    std::string notes;
};

class MarkdownParser {
public:
    static MarkdownParser& instance();
    bool parse_file(const std::string& path, Recipe& recipe) const;
    bool parse(const std::string& content, Recipe& recipe) const;
    std::vector<uint32_t> extract_timer_seconds(const std::string& text) const;

public:
    MarkdownParser() = default;
};

} // namespace services
