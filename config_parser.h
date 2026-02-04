/*
 * Simple JSON Configuration Parser for GPU-Burn
 * Lightweight implementation without external dependencies
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>

class ConfigParser {
private:
    std::map<std::string, std::string> config;

    // Simple JSON parser for key-value pairs
    // Supports: "key": value, "key": "value", comments with //
    void parseLine(const std::string& line) {
        std::string trimmed = line;

        // Remove whitespace
        trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
        trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

        // Skip empty lines, comments, braces
        if (trimmed.empty() || trimmed[0] == '{' || trimmed[0] == '}' ||
            trimmed.substr(0, 2) == "//") {
            return;
        }

        // Find key in quotes
        size_t keyStart = trimmed.find('"');
        if (keyStart == std::string::npos) return;

        size_t keyEnd = trimmed.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) return;

        std::string key = trimmed.substr(keyStart + 1, keyEnd - keyStart - 1);

        // Find colon
        size_t colonPos = trimmed.find(':', keyEnd);
        if (colonPos == std::string::npos) return;

        // Get value (after colon)
        std::string valueStr = trimmed.substr(colonPos + 1);
        valueStr.erase(0, valueStr.find_first_not_of(" \t"));

        // Remove trailing comma and whitespace
        size_t commaPos = valueStr.find(',');
        if (commaPos != std::string::npos) {
            valueStr = valueStr.substr(0, commaPos);
        }
        valueStr.erase(valueStr.find_last_not_of(" \t\n\r") + 1);

        // Remove quotes if present
        if (valueStr.size() >= 2 && valueStr[0] == '"' &&
            valueStr[valueStr.size() - 1] == '"') {
            valueStr = valueStr.substr(1, valueStr.size() - 2);
        }

        // Convert "true"/"false" to 1/0
        if (valueStr == "true") valueStr = "1";
        if (valueStr == "false") valueStr = "0";

        config[key] = valueStr;
    }

public:
    ConfigParser() {}

    bool loadFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            parseLine(line);
        }

        file.close();
        return true;
    }

    std::string getString(const std::string& key, const std::string& defaultValue = "") {
        auto it = config.find(key);
        if (it != config.end()) {
            return it->second;
        }
        return defaultValue;
    }

    int getInt(const std::string& key, int defaultValue = 0) {
        auto it = config.find(key);
        if (it != config.end()) {
            return std::stoi(it->second);
        }
        return defaultValue;
    }

    bool getBool(const std::string& key, bool defaultValue = false) {
        auto it = config.find(key);
        if (it != config.end()) {
            return it->second == "1" || it->second == "true";
        }
        return defaultValue;
    }

    bool hasKey(const std::string& key) {
        return config.find(key) != config.end();
    }

    void dump() {
        for (const auto& kv : config) {
            printf("  %s = %s\n", kv.c_str(), kv.second.c_str());
        }
    }
};

#endif // CONFIG_PARSER_H
