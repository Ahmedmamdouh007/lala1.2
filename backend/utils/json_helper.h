#pragma once

#include <string>
#include <sstream>

namespace json_helper {
    inline std::string escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else out += c;
        }
        return out;
    }
    inline std::string quote(const std::string& s) {
        return "\"" + escape(s) + "\"";
    }
    inline std::string int_to_str(int v) {
        return std::to_string(v);
    }
    inline std::string double_to_str(double v) {
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << v;
        return oss.str();
    }
}
