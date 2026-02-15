#pragma once

#include <string>
#include <cstddef>

namespace lab {
namespace validation_demo {

/// Result of analyzing user input for the validation demo.
struct AnalysisResult {
    size_t input_length = 0;
    bool is_dangerous = false;
    std::string reason;       // Why it's dangerous (or "Safe" / empty)
    std::string how_to_fix;   // Recommendation
};

/// Analyze input for demo: length, danger, and how to fix.
AnalysisResult analyze_input(const std::string& input);

// ---------------------------------------------------------------------------
// Bad validation examples (for teaching: what NOT to do)
// ---------------------------------------------------------------------------

/// BAD: No length check; can lead to buffer overflow or DoS.
inline bool bad_no_length_check(const std::string& s) {
    return true;  // Accepts everything
}

/// BAD: Concatenating input into SQL (vulnerable to injection).
inline std::string bad_concatenate_into_sql(const std::string& term) {
    return "SELECT * FROM products WHERE name LIKE '%" + term + "%'";
}

/// BAD: Only checking for one quote; easy to bypass.
inline bool bad_weak_quote_check(const std::string& s) {
    return s.find('\'') == std::string::npos;
}

// ---------------------------------------------------------------------------
// Correct validation examples (for teaching: what TO do)
// ---------------------------------------------------------------------------

constexpr size_t MAX_SAFE_INPUT_LEN = 500;

/// GOOD: Enforce maximum length before use.
inline bool correct_length_check(const std::string& s, size_t max_len = MAX_SAFE_INPUT_LEN) {
    return s.size() <= max_len;
}

/// GOOD: Use parameterized query (conceptually); never concatenate.
inline std::string correct_parameterized_usage() {
    return "SELECT * FROM products WHERE name ILIKE $1";  // $1 = bound parameter
}

/// GOOD: Allowlist / sanitize: reject control characters and trim.
inline bool correct_has_control_chars(const std::string& s) {
    for (unsigned char c : s) {
        if (c < 32 && c != '\t') return true;
    }
    return false;
}

} // namespace validation_demo
} // namespace lab
