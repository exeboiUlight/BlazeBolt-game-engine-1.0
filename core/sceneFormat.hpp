#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <cctype>

namespace BlazeBolt {

struct SceneObjectData {
    std::string name;
    std::string type;
    std::unordered_map<std::string, std::string> strings;
    std::unordered_map<std::string, double> numbers;
    std::unordered_map<std::string, bool> booleans;
    std::unordered_map<std::string, std::vector<double>> arrays;

    std::string getString(const std::string& key, const std::string& def = "") const {
        auto it = strings.find(key);
        return it != strings.end() ? it->second : def;
    }
    double getNumber(const std::string& key, double def = 0) const {
        auto it = numbers.find(key);
        return it != numbers.end() ? it->second : def;
    }
    bool getBool(const std::string& key, bool def = false) const {
        auto it = booleans.find(key);
        return it != booleans.end() ? it->second : def;
    }
    std::vector<double> getArray(const std::string& key) const {
        auto it = arrays.find(key);
        return it != arrays.end() ? it->second : std::vector<double>();
    }
};

struct SceneDocument {
    std::string name;
    int version = 1;
    std::vector<SceneObjectData> objects;

    std::string toJSON() const {
        std::ostringstream os;
        os.precision(6);
        os << "{\n";
        os << "  \"name\": " << escape(name) << ",\n";
        os << "  \"version\": " << version << ",\n";
        os << "  \"objects\": [\n";
        for (size_t i = 0; i < objects.size(); i++) {
            const auto& obj = objects[i];
            os << "    {\n";
            os << "      \"name\": " << escape(obj.name) << ",\n";
            os << "      \"type\": " << escape(obj.type);
            for (const auto& [k, v] : obj.strings)
                os << ",\n      " << escapeKey(k) << ": " << escape(v);
            for (const auto& [k, v] : obj.numbers)
                os << ",\n      " << escapeKey(k) << ": " << v;
            for (const auto& [k, v] : obj.booleans)
                os << ",\n      " << escapeKey(k) << ": " << (v ? "true" : "false");
            for (const auto& [k, v] : obj.arrays) {
                os << ",\n      " << escapeKey(k) << ": [";
                for (size_t j = 0; j < v.size(); j++) {
                    if (j > 0) os << ", ";
                    os << v[j];
                }
                os << "]";
            }
            os << "\n    }";
            if (i < objects.size() - 1) os << ",";
            os << "\n";
        }
        os << "  ]\n}\n";
        return os.str();
    }

    static SceneDocument fromJSON(const std::string& json) {
        SceneDocument doc;
        size_t pos = 0;
        skipWS(json, pos);
        if (pos >= json.size() || json[pos] != '{') return doc;
        pos++;

        while (pos < json.size()) {
            skipWS(json, pos);
            if (pos >= json.size() || json[pos] == '}') break;
            std::string key = parseString(json, pos);
            skipWS(json, pos);
            if (pos < json.size() && json[pos] == ':') pos++;
            skipWS(json, pos);

            if (key == "name") {
                doc.name = parseString(json, pos);
            } else if (key == "version") {
                doc.version = static_cast<int>(parseNumber(json, pos));
            } else if (key == "objects") {
                skipWS(json, pos);
                if (pos < json.size() && json[pos] == '[') pos++;
                while (pos < json.size()) {
                    skipWS(json, pos);
                    if (pos >= json.size() || json[pos] == ']') break;
                    if (json[pos] == '{') {
                        doc.objects.push_back(parseObject(json, pos));
                    } else {
                        pos++;
                    }
                }
                if (pos < json.size() && json[pos] == ']') pos++;
            } else {
                skipValue(json, pos);
            }
            skipWS(json, pos);
            if (pos < json.size() && json[pos] == ',') pos++;
        }
        return doc;
    }

private:
    static std::string escape(const std::string& s) {
        std::string r;
        r += '"';
        for (char c : s) {
            if (c == '"') r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else if (c == '\n') r += "\\n";
            else if (c == '\t') r += "\\t";
            else r += c;
        }
        r += '"';
        return r;
    }

    static std::string escapeKey(const std::string& k) {
        return "\"" + k + "\"";
    }

    static void skipWS(const std::string& s, size_t& pos) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
            pos++;
    }

    static std::string parseString(const std::string& s, size_t& pos) {
        skipWS(s, pos);
        if (pos >= s.size() || s[pos] != '"') return "";
        pos++;
        std::string r;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\') {
                pos++;
                if (pos < s.size()) {
                    switch (s[pos]) {
                        case '"': r += '"'; break;
                        case '\\': r += '\\'; break;
                        case 'n': r += '\n'; break;
                        case 't': r += '\t'; break;
                        default: r += s[pos]; break;
                    }
                }
            } else {
                r += s[pos];
            }
            pos++;
        }
        if (pos < s.size()) pos++;
        return r;
    }

    static double parseNumber(const std::string& s, size_t& pos) {
        skipWS(s, pos);
        size_t start = pos;
        if (pos < s.size() && s[pos] == '-') pos++;
        while (pos < s.size() && (std::isdigit(s[pos]) || s[pos] == '.')) pos++;
        if (pos < s.size() && (s[pos] == 'e' || s[pos] == 'E')) {
            pos++;
            if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) pos++;
            while (pos < s.size() && std::isdigit(s[pos])) pos++;
        }
        std::string num = s.substr(start, pos - start);
        return num.empty() ? 0 : std::stod(num);
    }

    static bool parseBool(const std::string& s, size_t& pos) {
        skipWS(s, pos);
        if (s.substr(pos, 4) == "true") { pos += 4; return true; }
        if (s.substr(pos, 5) == "false") { pos += 5; return false; }
        return false;
    }

    static std::vector<double> parseArray(const std::string& s, size_t& pos) {
        std::vector<double> r;
        skipWS(s, pos);
        if (pos >= s.size() || s[pos] != '[') return r;
        pos++;
        while (pos < s.size()) {
            skipWS(s, pos);
            if (pos >= s.size() || s[pos] == ']') break;
            if (s[pos] == '"') { parseString(s, pos); }
            else if (s[pos] == '-' || std::isdigit(s[pos]) || s[pos] == '.') {
                r.push_back(parseNumber(s, pos));
            } else {
                pos++;
            }
            skipWS(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
        }
        if (pos < s.size() && s[pos] == ']') pos++;
        return r;
    }

    static void skipValue(const std::string& s, size_t& pos) {
        skipWS(s, pos);
        if (pos >= s.size()) return;
        if (s[pos] == '"') { parseString(s, pos); }
        else if (s[pos] == '{') { int d = 1; pos++; while (pos < s.size() && d > 0) { if (s[pos] == '{') d++; else if (s[pos] == '}') d--; pos++; } }
        else if (s[pos] == '[') { int d = 1; pos++; while (pos < s.size() && d > 0) { if (s[pos] == '[') d++; else if (s[pos] == ']') d--; pos++; } }
        else if (s.substr(pos, 4) == "true" || s.substr(pos, 5) == "false") { while (pos < s.size() && std::isalpha(s[pos])) pos++; }
        else if (s.substr(pos, 4) == "null") { pos += 4; }
        else { while (pos < s.size() && s[pos] != ',' && s[pos] != '}' && s[pos] != ']' && !std::isspace(s[pos])) pos++; }
    }

    static SceneObjectData parseObject(const std::string& s, size_t& pos) {
        SceneObjectData obj;
        skipWS(s, pos);
        if (pos < s.size() && s[pos] == '{') pos++;
        while (pos < s.size()) {
            skipWS(s, pos);
            if (pos >= s.size() || s[pos] == '}') break;
            std::string key = parseString(s, pos);
            skipWS(s, pos);
            if (pos < s.size() && s[pos] == ':') pos++;
            skipWS(s, pos);

            if (pos >= s.size()) break;
            if (s[pos] == '"') {
                std::string val = parseString(s, pos);
                if (key == "name") obj.name = val;
                else if (key == "type") obj.type = val;
                else obj.strings[key] = val;
            } else if (s[pos] == '-' || std::isdigit(s[pos]) || s[pos] == '.') {
                obj.numbers[key] = parseNumber(s, pos);
            } else if (s.substr(pos, 4) == "true" || s.substr(pos, 5) == "false") {
                obj.booleans[key] = parseBool(s, pos);
            } else if (s[pos] == '[') {
                obj.arrays[key] = parseArray(s, pos);
            } else {
                skipValue(s, pos);
            }
            skipWS(s, pos);
            if (pos < s.size() && s[pos] == ',') pos++;
        }
        if (pos < s.size() && s[pos] == '}') pos++;
        return obj;
    }
};

} // namespace BlazeBolt
