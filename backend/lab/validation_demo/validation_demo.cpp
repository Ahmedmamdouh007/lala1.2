#include "validation_demo.h"
#include <algorithm>
#include <cctype>

namespace lab {
namespace validation_demo {

AnalysisResult analyze_input(const std::string& input) {
    AnalysisResult r;
    r.input_length = input.size();

    if (input.empty()) {
        r.is_dangerous = false;
        r.reason = "Empty input.";
        r.how_to_fix = "No fix needed. Consider requiring non-empty for required fields.";
        return r;
    }

    std::string reasons;
    std::string fixes;

    // Length
    if (input.size() > MAX_SAFE_INPUT_LEN) {
        r.is_dangerous = true;
        if (!reasons.empty()) reasons += " ";
        reasons += "Input length " + std::to_string(input.size()) + " exceeds safe limit (" + std::to_string(MAX_SAFE_INPUT_LEN) + "); can cause buffer overflow or DoS.";
        if (!fixes.empty()) fixes += " ";
        fixes += "Enforce max length (e.g. " + std::to_string(MAX_SAFE_INPUT_LEN) + ") before use; use correct_length_check().";
    }

    // Control characters
    if (correct_has_control_chars(input)) {
        r.is_dangerous = true;
        if (!reasons.empty()) reasons += " ";
        reasons += "Contains control characters (e.g. newline, null); can break parsing or inject.";
        if (!fixes.empty()) fixes += " ";
        fixes += "Reject or strip control chars; validate with allowlist.";
    }

    // SQL-like (quotes / injection patterns)
    bool has_quote = input.find('\'') != std::string::npos || input.find('"') != std::string::npos;
    std::string lower = input;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    bool has_sql = lower.find("select") != std::string::npos || lower.find("union") != std::string::npos ||
                   lower.find("or 1=1") != std::string::npos || lower.find("--") != std::string::npos;
    if (has_quote || has_sql) {
        r.is_dangerous = true;
        if (!reasons.empty()) reasons += " ";
        reasons += "Contains quotes or SQL-like tokens; dangerous if concatenated into SQL.";
        if (!fixes.empty()) fixes += " ";
        fixes += "Use parameterized queries (e.g. exec_params with $1); never concatenate input into SQL.";
    }

    // Script-like (XSS)
    if (lower.find("<script") != std::string::npos || lower.find("javascript:") != std::string::npos ||
        lower.find("onerror=") != std::string::npos) {
        r.is_dangerous = true;
        if (!reasons.empty()) reasons += " ";
        reasons += "Contains script-like content; dangerous if reflected in HTML without escaping.";
        if (!fixes.empty()) fixes += " ";
        fixes += "Escape output for HTML/JS context; use Content-Security-Policy.";
    }

    if (r.is_dangerous) {
        r.reason = reasons;
        r.how_to_fix = fixes;
    } else {
        r.reason = "Safe for demo: length within limit, no obvious injection or control chars.";
        r.how_to_fix = "Still use parameterized queries and output encoding in production.";
    }

    return r;
}

} // namespace validation_demo
} // namespace lab
