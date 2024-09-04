#ifndef JSON_HPP
#define JSON_HPP

#include <iostream>
#include <unordered_map>
#include <string>

// Function to escape special characters for JSON
std::string escapeJsonString(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        switch (ch) {
            case '\"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '/':  result += "\\/";  break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default: result += ch; break;
        }
    }
    return result;
}

// Function to convert unordered_map to JSON string
std::string unorderedMapToJson(const std::unordered_map<std::string, std::string>& map) {
    std::string json = "{";
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin()) {
            json += ", ";
        }
        json += "\"" + escapeJsonString(it->first) + "\": \"" + escapeJsonString(it->second) + "\"";
    }
    json += "}";
    return json;
}

// Function to trim whitespace from the beginning and end of a string
std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r\f\v");
    auto end = str.find_last_not_of(" \t\n\r\f\v");
    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// Function to parse a JSON string into an unordered_map
std::unordered_map<std::string, std::string> jsonToUnorderedMap(const std::string& json) {
    std::unordered_map<std::string, std::string> map;
    size_t pos = 0;
    size_t length = json.length();
    
    // Skip the opening brace
    if (pos < length && json[pos] == '{') {
        ++pos;
    }

    while (pos < length) {
        // Skip whitespace
        while (pos < length && std::isspace(json[pos])) {
            ++pos;
        }

        // Check for closing brace
        if (pos < length && json[pos] == '}') {
            ++pos;
            break;
        }

        // Parse key
        if (pos < length && json[pos] == '\"') {
            ++pos;
            size_t key_start = pos;

            while (pos < length && json[pos] != '\"') {
                ++pos;
            }

            std::string key = json.substr(key_start, pos - key_start);
            ++pos; // Skip closing quote

            // Skip whitespace and colon
            while (pos < length && std::isspace(json[pos])) {
                ++pos;
            }

            if (pos < length && json[pos] == ':') {
                ++pos;
            }

            // Skip whitespace
            while (pos < length && std::isspace(json[pos])) {
                ++pos;
            }

            // Parse value
            if (pos < length && json[pos] == '\"') {
                ++pos;
                size_t value_start = pos;
                while (pos < length && json[pos] != '\"') {
                    ++pos;
                }
                std::string value = json.substr(value_start, pos - value_start);
                ++pos; // Skip closing quote
                map[trim(key)] = trim(value);
            }

            // Skip whitespace and comma
            while (pos < length && std::isspace(json[pos])) {
                ++pos;
            }

            if (pos < length && json[pos] == ',') {
                ++pos;
            }
        }
    }

    return map;
}
#endif
