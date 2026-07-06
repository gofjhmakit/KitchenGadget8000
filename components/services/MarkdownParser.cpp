#include "services/MarkdownParser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace services {
namespace {

std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split_tags(const std::string& raw) {
    std::string body = raw;
    if (!body.empty() && body.front() == '[') body.erase(body.begin());
    if (!body.empty() && body.back() == ']')  body.pop_back();
    std::vector<std::string> tags;
    std::stringstream ss(body);
    std::string item;
    while (std::getline(ss, item, ',')) {
        const auto t = trim(item);
        if (!t.empty()) tags.push_back(t);
    }
    return tags;
}

/**
 * Extract embedded timer durations from a step string without std::regex.
 * Matches patterns like: "14 min", "1 hour", "30 minutes".
 */
std::vector<uint32_t> extract_timers_from_step(const std::string& text) {
    std::vector<uint32_t> timers;
    const size_t len = text.size();
    size_t i = 0;
    while (i < len) {
        // Skip non-digit characters
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) { ++i; continue; }
        // Collect digits
        size_t j = i;
        while (j < len && std::isdigit(static_cast<unsigned char>(text[j]))) ++j;
        const uint32_t value = static_cast<uint32_t>(std::stoul(text.substr(i, j - i)));
        // Skip whitespace
        size_t k = j;
        while (k < len && (text[k] == ' ' || text[k] == '\t')) ++k;
        // Match unit keyword
        const std::string rest = text.substr(k);
        const char* unit_begin = rest.c_str();
        bool is_hours = false;
        bool matched = false;
        // Check for hour variants first (so "hour" doesn't match "h" stub)
        static const char* kHourWords[] = {"hours", "hour", "hrs", "hr", nullptr};
        static const char* kMinWords[]  = {"minutes", "minute", "mins", "min", nullptr};
        for (int w = 0; kHourWords[w] != nullptr; ++w) {
            const size_t wl = std::strlen(kHourWords[w]);
            if (::strncasecmp(unit_begin, kHourWords[w], wl) == 0 &&
                (rest.size() == wl || !std::isalpha(static_cast<unsigned char>(rest[wl])))) {
                is_hours = true;
                matched  = true;
                break;
            }
        }
        if (!matched) {
            for (int w = 0; kMinWords[w] != nullptr; ++w) {
                const size_t wl = std::strlen(kMinWords[w]);
                if (::strncasecmp(unit_begin, kMinWords[w], wl) == 0 &&
                    (rest.size() == wl || !std::isalpha(static_cast<unsigned char>(rest[wl])))) {
                    matched = true;
                    break;
                }
            }
        }
        if (matched && value > 0) {
            timers.push_back(is_hours ? value * 3600U : value * 60U);
        }
        i = j + 1;
    }
    return timers;
}

} // namespace

MarkdownParser& MarkdownParser::instance() {
    static MarkdownParser inst;
    return inst;
}

bool MarkdownParser::parse_file(const std::string& path, Recipe& recipe) const {
    std::ifstream file(path);
    if (!file) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    recipe.source_path = path;
    if (!parse(ss.str(), recipe)) return false;

    // Derive SPIFFS image path: same location + filename, .md → .jpg
    if (path.size() > 3) {
        const std::string suffix = ".md";
        if (path.size() >= suffix.size() &&
            path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0) {
            recipe.image_path = path.substr(0, path.size() - suffix.size()) + ".jpg";
        }
    }
    return true;
}

bool MarkdownParser::parse(const std::string& content, Recipe& recipe) const {
    recipe = Recipe{};
    std::istringstream ss(content);
    std::string line;
    bool in_frontmatter = false;
    bool first = true;
    enum class Section { NONE, INGREDIENTS, INSTRUCTIONS, NOTES } section = Section::NONE;

    while (std::getline(ss, line)) {
        const std::string t = trim(line);

        if (first && t == "---") {
            in_frontmatter = true;
            first = false;
            continue;
        }
        first = false;

        if (in_frontmatter) {
            if (t == "---") { in_frontmatter = false; continue; }
            const auto colon = t.find(':');
            if (colon == std::string::npos) continue;
            const std::string key   = trim(t.substr(0, colon));
            const std::string value = trim(t.substr(colon + 1));
            if      (key == "title")    recipe.title    = value;
            else if (key == "time")     recipe.time     = value;
            else if (key == "servings") recipe.servings = std::atoi(value.c_str());
            else if (key == "tags")     recipe.tags     = split_tags(value);
            else if (key == "image")    recipe.image    = value;
            continue;
        }

        if (t == "## Ingredients")   { section = Section::INGREDIENTS;  continue; }
        if (t == "## Instructions")  { section = Section::INSTRUCTIONS; continue; }
        if (t == "## Notes")         { section = Section::NOTES;        continue; }
        if (t.empty()) continue;

        switch (section) {
            case Section::INGREDIENTS:
                if (t.rfind("- ", 0) == 0) recipe.ingredients.push_back(t.substr(2));
                break;
            case Section::INSTRUCTIONS:
                if (!t.empty() && std::isdigit(static_cast<unsigned char>(t[0]))) {
                    const auto dot = t.find('.');
                    if (dot != std::string::npos)
                        recipe.instructions.push_back(trim(t.substr(dot + 1)));
                    else
                        recipe.instructions.push_back(t);
                } else {
                    recipe.instructions.push_back(t);
                }
                break;
            case Section::NOTES:
                if (!recipe.notes.empty()) recipe.notes += '\n';
                recipe.notes += t;
                break;
            default:
                break;
        }
    }
    return !recipe.title.empty();
}

std::vector<uint32_t> MarkdownParser::extract_timer_seconds(const std::string& text) const {
    return extract_timers_from_step(text);
}

} // namespace services

