#pragma once

#include "crow.h"
#include <optional>

namespace lab {

/**
 * Middleware guard for lab routes.
 * - If LAB_MODE is not true => return 404 (Not Found).
 * - If request is not from localhost (127.0.0.1 / ::1) => return 403 (Forbidden).
 * Otherwise returns std::nullopt (caller should proceed with the handler).
 */
inline std::optional<crow::response> guard(const crow::request& req, bool lab_mode_enabled) {
    if (!lab_mode_enabled)
        return crow::response(404, "application/json", "{\"error\":\"Not found\"}");
    const std::string& ip = req.remote_ip_address;
    if (ip != "127.0.0.1" && ip != "::1")
        return crow::response(403, "application/json",
            "{\"success\":false,\"error\":\"Lab endpoints are only available from localhost (127.0.0.1).\",\"lab_mode\":true}");
    return std::nullopt;
}

} // namespace lab
