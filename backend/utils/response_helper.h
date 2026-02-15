#pragma once

#include <string>
#include "json_helper.h"

namespace response_helper {
    inline std::string success_json(const std::string& data = "{}") {
        return "{\"success\":true,\"data\":" + data + "}";
    }
    inline std::string error_json(const std::string& message) {
        return "{\"success\":false,\"error\":\"" + json_helper::escape(message) + "\"}";
    }
    inline std::string success_message(const std::string& message) {
        return "{\"success\":true,\"message\":\"" + json_helper::escape(message) + "\"}";
    }
}
