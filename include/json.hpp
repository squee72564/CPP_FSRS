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

#endif
