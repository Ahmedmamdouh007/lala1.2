#pragma once

#include <string>

namespace lab {
namespace telemetry {

/// Log a lab endpoint request. Params string must already have sensitive
/// values (e.g. password) redacted — do not pass passwords here.
/// injection_pattern: e.g. "time_based", "error_based", "boolean_true", "auth_bypass", "none".
void log_request(const std::string& endpoint,
                 const std::string& params_redacted,
                 const std::string& injection_pattern,
                 int response_code);

/// Optional: set log file path (default: "logs/lab.log" relative to cwd).
void set_log_path(const std::string& path);

} // namespace telemetry
} // namespace lab
