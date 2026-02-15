#include "lab/telemetry/lab_telemetry.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <sys/stat.h>
#include <cerrno>
#endif

namespace lab {
namespace telemetry {

namespace {

std::string g_log_path = "logs/lab.log";
std::mutex g_log_mutex;

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void ensure_log_dir() {
#if __cplusplus >= 201703L
    fs::path p(g_log_path);
    if (p.has_parent_path()) {
        try {
            fs::create_directories(p.parent_path());
        } catch (...) {}
    }
#else
    (void)g_log_path;
#endif
}

} // namespace

void set_log_path(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    g_log_path = path;
}

void log_request(const std::string& endpoint,
                 const std::string& params_redacted,
                 const std::string& injection_pattern,
                 int response_code) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    ensure_log_dir();
    std::ofstream out(g_log_path, std::ios::app);
    if (!out) return;
    out << current_timestamp()
        << " endpoint=" << endpoint
        << " params=" << (params_redacted.empty() ? "(none)" : params_redacted)
        << " injection=" << injection_pattern
        << " response=" << response_code
        << "\n";
}

} // namespace telemetry
} // namespace lab
