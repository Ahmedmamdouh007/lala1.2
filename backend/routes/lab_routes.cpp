#include "crow.h"
#include "../db/connection.h"
#include "../utils/json_helper.h"
#include "lab/lab_guard.h"
#include "lab/telemetry/lab_telemetry.h"
#include "lab/validation_demo/validation_demo.h"
#include <pqxx/pqxx>
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace lab_routes {

namespace {

std::string lab_warning_prefix() {
    return "\"lab_mode\":true,\"warning\":\"Do not deploy this\",\"training_lab\":\"Unsafe query building example - use parameterized queries in production\"";
}

// Simulate time-based SQLi detection: if input looks like a sleep payload, delay and return response time (no real DB sleep).
bool looks_like_time_based_payload(const std::string& s) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return lower.find("sleep") != std::string::npos ||
           lower.find("pg_sleep") != std::string::npos ||
           lower.find("benchmark") != std::string::npos ||
           lower.find("waitfor") != std::string::npos;
}

// Response time in ms (for teaching: observable difference when "time-based" payload is used).
constexpr unsigned int SIMULATED_DELAY_MS = 1000;

std::string to_lower(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

// Detect error-based style payloads (quote, or common error-triggering patterns).
bool looks_like_error_based_payload(const std::string& s) {
    std::string lower = to_lower(s);
    return lower.find('\'') != std::string::npos || lower.find('\"') != std::string::npos ||
           lower.find("extractvalue") != std::string::npos || lower.find("updatexml") != std::string::npos ||
           lower.find("exp(") != std::string::npos || lower.find("convert(") != std::string::npos;
}

// Detect boolean-based style payloads.
bool looks_like_boolean_true(const std::string& s) {
    std::string lower = to_lower(s);
    return lower.find("1=1") != std::string::npos || lower.find("'1'='1'") != std::string::npos ||
           lower.find(" and true") != std::string::npos || lower.find(" or true") != std::string::npos;
}
bool looks_like_boolean_false(const std::string& s) {
    std::string lower = to_lower(s);
    return lower.find("1=2") != std::string::npos || lower.find(" and false") != std::string::npos ||
           lower.find(" or false") != std::string::npos;
}

// Detect union-based style payloads.
bool looks_like_union_payload(const std::string& s) {
    std::string lower = to_lower(s);
    return lower.find("union") != std::string::npos && (lower.find("select") != std::string::npos || lower.find("all") != std::string::npos);
}

// Detect auth-bypass style payloads in email or password.
bool looks_like_auth_bypass(const std::string& s) {
    std::string lower = to_lower(s);
    return lower.find("' or '1'='1") != std::string::npos || lower.find("' or 1=1") != std::string::npos ||
           lower.find("or 1=1--") != std::string::npos || lower.find("' or 1=1--") != std::string::npos ||
           lower.find("admin'--") != std::string::npos || lower.find("'--") != std::string::npos ||
           lower.find("\" or \"1\"=\"1") != std::string::npos;
}

// Build query params string with password/pass values redacted (never log passwords).
std::string build_params_redacted(const crow::request& req) {
    const std::string& raw = req.raw_url;
    size_t q = raw.find('?');
    if (q == std::string::npos || q + 1 >= raw.size()) return "";
    std::string query = raw.substr(q + 1);
    std::ostringstream out;
    size_t pos = 0;
    bool first = true;
    while (pos < query.size()) {
        size_t amp = query.find('&', pos);
        std::string pair = (amp == std::string::npos) ? query.substr(pos) : query.substr(pos, amp - pos);
        pos = (amp == std::string::npos) ? query.size() : amp + 1;
        size_t eq = pair.find('=');
        std::string key = (eq == std::string::npos) ? pair : pair.substr(0, eq);
        std::string val = (eq == std::string::npos) ? "" : pair.substr(eq + 1);
        std::string key_lower = to_lower(key);
        if (key_lower == "password" || key_lower == "pass" || key_lower == "pwd")
            val = "[REDACTED]";
        if (!first) out << "&";
        first = false;
        out << key << "=" << val;
    }
    return out.str();
}

// Build JSON for a product row (products + category name only; no users table).
std::string product_row_to_json(const pqxx::row& row) {
    int id = row[0].as<int>();
    int cat_id = row[1].as<int>();
    std::string name = row[2].as<std::string>();
    std::string desc = row[3].is_null() ? "" : row[3].as<std::string>();
    double price = row[4].as<double>();
    std::string img = row[5].is_null() ? "" : row[5].as<std::string>();
    int stock = row[6].as<int>();
    std::string cat_name = row[7].is_null() ? "" : row[7].as<std::string>();
    std::string created = row[8].is_null() ? "" : row[8].as<std::string>();
    return "{\"id\":" + std::to_string(id) +
        ",\"category_id\":" + std::to_string(cat_id) +
        ",\"name\":" + json_helper::quote(name) +
        ",\"description\":" + json_helper::quote(desc) +
        ",\"price\":" + json_helper::double_to_str(price) +
        ",\"image_url\":" + json_helper::quote(img) +
        ",\"stock\":" + std::to_string(stock) +
        ",\"category_name\":" + json_helper::quote(cat_name) +
        ",\"created_at\":" + json_helper::quote(created) + "}";
}

} // namespace

void register_routes(crow::SimpleApp& app, bool lab_mode_enabled) {
    // --- Training lab: SQLi search (unsafe query building example) ---
    // Protected by: read-only DB role, no users table, query timeout, max 1 query per request.
    CROW_ROUTE(app, "/lab/sqli/search")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* termParam = req.url_params.get("term");
        std::string term = termParam ? termParam : "";
        // Limit length to reduce abuse
        if (term.size() > 200) term = term.substr(0, 200);

        // Time-based SQLi simulation (teaching): detect sleep-like payloads, simulate delay without running DB sleep.
        if (looks_like_time_based_payload(term)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATED_DELAY_MS));
            std::string body = "{" + lab_warning_prefix() +
                ",\"data\":[],\"response_time_ms\":" + std::to_string(SIMULATED_DELAY_MS) +
                ",\"simulated_time_based_sqli\":true,\"lab_message\":\"Time-based SQLi simulation: payload containing sleep/pg_sleep/benchmark detected. Response delayed by " +
                std::to_string(SIMULATED_DELAY_MS) + " ms for teaching. No dangerous DB functions were executed.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "time_based", 200);
            return crow::response(200, "application/json", body);
        }

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            // Query timeout: 5 seconds (lab safety)
            txn.exec("SET statement_timeout = 5000");
            // UNSAFE QUERY BUILDING (training example): concatenating input into SQL.
            // In production always use parameterized queries (e.g. exec_params with $1).
            // We only query products/categories - no access to users table.
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE '%" + term + "%' OR p.description ILIKE '%" + term + "%' ORDER BY p.id LIMIT 50";
            auto r = txn.exec(sql);
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_row_to_json(r[i]);
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"response_time_ms\":0}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() +
                ",\"success\":false,\"error\":" + json_helper::quote(err) +
                ",\"lab_message\":\"Lab: This error is shown for teaching. Use parameterized queries (e.g. exec_params with $1) to avoid injection. Error: " + json_helper::escape(err) + "\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- Training lab: SQLi product by id (unsafe query building example) ---
    // Same restrictions: read-only, no users table, query timeout, max 1 query.
    CROW_ROUTE(app, "/lab/sqli/product")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* idParam = req.url_params.get("id");
        if (!idParam || *idParam == '\0') {
            std::string body = "{" + lab_warning_prefix() +
                ",\"success\":false,\"error\":\"Missing id parameter\",\"lab_message\":\"Lab: Always validate required parameters before building queries.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 400);
            return crow::response(400, "application/json", body);
        }
        std::string idStr(idParam);
        if (idStr.size() > 20) idStr = idStr.substr(0, 20);

        // Time-based SQLi simulation (teaching): same as search.
        if (looks_like_time_based_payload(idStr)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATED_DELAY_MS));
            std::string body = "{" + lab_warning_prefix() +
                ",\"data\":null,\"response_time_ms\":" + std::to_string(SIMULATED_DELAY_MS) +
                ",\"simulated_time_based_sqli\":true,\"lab_message\":\"Time-based SQLi simulation: payload detected in id. Response delayed for teaching. No dangerous DB functions executed.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "time_based", 200);
            return crow::response(200, "application/json", body);
        }

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            // UNSAFE: concatenating id into SQL (training example). Use exec_params($1) in production.
            // Only products/categories - no users table.
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id WHERE p.id = " + idStr;
            auto r = txn.exec(sql);
            txn.commit();

            if (r.empty()) {
                std::string body = "{" + lab_warning_prefix() + ",\"data\":null,\"message\":\"Product not found\",\"lab_message\":\"Lab: No row returned; id may be invalid or injected.\"}";
                lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 404);
                return crow::response(404, "application/json", body);
            }
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + product_row_to_json(r[0]) + ",\"response_time_ms\":0}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() +
                ",\"success\":false,\"error\":" + json_helper::quote(err) +
                ",\"lab_message\":\"Lab: This error is shown for teaching. Use parameterized queries to avoid injection. Error: " + json_helper::escape(err) + "\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 1. Error-based SQLi training: simulate DB error leakage ---
    CROW_ROUTE(app, "/lab/sqli/error_based")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* termParam = req.url_params.get("term");
        std::string term = termParam ? termParam : "";
        if (term.size() > 200) term = term.substr(0, 200);

        if (looks_like_error_based_payload(term)) {
            // Simulate error-based SQLi: return a fake DB-style error (no real dangerous query).
            std::string fake_error = "ERROR: syntax error at or near \"'\"; Unclosed quote in term. (Simulated for training - no real query executed.)";
            std::string body = "{" + lab_warning_prefix() +
                ",\"success\":false,\"sqli_type\":\"error_based\",\"error\":" + json_helper::quote(fake_error) +
                ",\"lab_message\":\"Error-based SQLi simulation: payload triggered simulated DB error. In a real vulnerability, error messages can leak schema or data.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "error_based", 500);
            return crow::response(500, "application/json", body);
        }

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE '%" + term + "%' OR p.description ILIKE '%" + term + "%' ORDER BY p.id LIMIT 50";
            auto r = txn.exec(sql);
            txn.commit();
            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_row_to_json(r[i]);
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"error_based\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() +
                ",\"success\":false,\"sqli_type\":\"error_based\",\"error\":" + json_helper::quote(err) +
                ",\"lab_message\":\"Real error from concatenated query (teaching). Use parameterized queries.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 2. Boolean-based SQLi training: different result sizes for true/false conditions ---
    CROW_ROUTE(app, "/lab/sqli/boolean_based")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* termParam = req.url_params.get("term");
        std::string term = termParam ? termParam : "";
        if (term.size() > 200) term = term.substr(0, 200);

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE '%" + term + "%' OR p.description ILIKE '%" + term + "%' ORDER BY p.id LIMIT 50";
            auto r = txn.exec(sql);
            txn.commit();

            bool sim_true = looks_like_boolean_true(term);
            bool sim_false = looks_like_boolean_false(term);
            std::string arr = "[";
            if (sim_false && !sim_true) {
                // Simulate boolean false: return empty result even if query would return rows.
                arr += "]";
                std::string body = "{" + lab_warning_prefix() +
                    ",\"data\":[],\"sqli_type\":\"boolean_based\",\"simulated\":\"false_condition\",\"count\":0"
                    ",\"lab_message\":\"Boolean-based SQLi simulation: false condition payload detected; returned empty to simulate different page behavior.\"}";
                lab::telemetry::log_request(req.url, build_params_redacted(req), "boolean_false", 200);
                return crow::response(200, "application/json", body);
            }
            if (sim_true) {
                // Simulate boolean true: return full set (already have r).
                for (size_t i = 0; i < r.size(); i++) {
                    if (i > 0) arr += ",";
                    arr += product_row_to_json(r[i]);
                }
                arr += "]";
                std::string body = "{" + lab_warning_prefix() +
                    ",\"data\":" + arr + ",\"sqli_type\":\"boolean_based\",\"simulated\":\"true_condition\",\"count\":" + std::to_string(r.size()) +
                    ",\"lab_message\":\"Boolean-based SQLi simulation: true condition payload detected; full result set returned.\"}";
                lab::telemetry::log_request(req.url, build_params_redacted(req), "boolean_true", 200);
                return crow::response(200, "application/json", body);
            }

            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_row_to_json(r[i]);
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"boolean_based\",\"count\":" + std::to_string(r.size()) + "}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() + ",\"success\":false,\"error\":" + json_helper::quote(err) + "}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 3. Time-based SQLi training: simulate delay when sleep-like payload ---
    CROW_ROUTE(app, "/lab/sqli/time_based")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* termParam = req.url_params.get("term");
        std::string term = termParam ? termParam : "";
        if (term.size() > 200) term = term.substr(0, 200);

        if (looks_like_time_based_payload(term)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATED_DELAY_MS));
            std::string body = "{" + lab_warning_prefix() +
                ",\"data\":[],\"sqli_type\":\"time_based\",\"response_time_ms\":" + std::to_string(SIMULATED_DELAY_MS) +
                ",\"simulated_delay\":true,\"lab_message\":\"Time-based SQLi: sleep-like payload detected. Response delayed by " +
                std::to_string(SIMULATED_DELAY_MS) + " ms for teaching. No DB sleep executed.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "time_based", 200);
            return crow::response(200, "application/json", body);
        }

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE '%" + term + "%' OR p.description ILIKE '%" + term + "%' ORDER BY p.id LIMIT 50";
            auto r = txn.exec(sql);
            txn.commit();
            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_row_to_json(r[i]);
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"time_based\",\"response_time_ms\":0}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() + ",\"success\":false,\"error\":" + json_helper::quote(err) + "}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 4. Union-based SQLi training: simulate extra rows/columns when union-like payload ---
    CROW_ROUTE(app, "/lab/sqli/union_based")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* termParam = req.url_params.get("term");
        std::string term = termParam ? termParam : "";
        if (term.size() > 200) term = term.substr(0, 200);

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            std::string sql = "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE '%" + term + "%' OR p.description ILIKE '%" + term + "%' ORDER BY p.id LIMIT 50";
            auto r = txn.exec(sql);
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_row_to_json(r[i]);
            }
            bool union_detected = looks_like_union_payload(term);
            if (union_detected) {
                // Simulate union-based: inject a fake "leaked" row (no real UNION executed).
                if (r.size() > 0) arr += ",";
                arr += "{\"id\":-1,\"category_id\":0,\"name\":\"[UNION LEAK SIMULATION]\",\"description\":\"Fake row for training. Real union-based SQLi could leak data from other tables.\",\"price\":0,\"image_url\":\"\",\"stock\":0,\"category_name\":\"\",\"created_at\":\"\"}";
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"union_based\"";
            if (union_detected)
                body += ",\"simulated_union_row\":true,\"lab_message\":\"Union-based SQLi simulation: UNION-like payload detected. Extra row added for teaching; no real UNION executed.\"";
            body += "}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), union_detected ? "union_based" : "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() + ",\"success\":false,\"error\":" + json_helper::quote(err) + "}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 5. Auth-bypass SQLi training: simulate login success when bypass payload in email/password ---
    CROW_ROUTE(app, "/lab/sqli/auth_bypass")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* emailParam = req.url_params.get("email");
        const char* passwordParam = req.url_params.get("password");
        std::string email = emailParam ? emailParam : "";
        std::string password = passwordParam ? passwordParam : "";
        if (email.size() > 200) email = email.substr(0, 200);
        if (password.size() > 200) password = password.substr(0, 200);

        bool bypass = looks_like_auth_bypass(email) || looks_like_auth_bypass(password);

        if (bypass) {
            // Simulate auth bypass: return fake "logged in" (no real auth or users table).
            std::string body = "{" + lab_warning_prefix() +
                ",\"sqli_type\":\"auth_bypass\",\"success\":true,\"simulated_bypass\":true,"
                "\"user\":{\"id\":1,\"email\":\"admin@lab.local\",\"name\":\"[Simulated Admin]\"},"
                "\"lab_message\":\"Auth-bypass SQLi simulation: classic bypass payload detected (e.g. ' OR '1'='1). No real login or users table accessed.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "auth_bypass", 200);
            return crow::response(200, "application/json", body);
        }

        // Normal: simulate failed login (we do not touch real users table).
        std::string body = "{" + lab_warning_prefix() +
            ",\"sqli_type\":\"auth_bypass\",\"success\":false,\"error\":\"Invalid email or password\","
            "\"lab_message\":\"No bypass payload detected. Try classic payloads in email or password for training.\"}";
        lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 401);
        return crow::response(401, "application/json", body);
    });

    // --- 6. Order-by SQLi training: concatenate column into ORDER BY (unsafe) ---
    CROW_ROUTE(app, "/lab/sqli/order_by")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* colParam = req.url_params.get("column");
        std::string column = colParam ? colParam : "p.id";
        if (column.size() > 100) column = column.substr(0, 100);

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            // UNSAFE: concatenating user input into ORDER BY clause.
            std::string sql = "SELECT p.id, p.name, p.price FROM products p ORDER BY " + column + " LIMIT 20";
            auto r = txn.exec(sql);
            txn.commit();
            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += "{\"id\":" + r[i][0].as<std::string>() + ",\"name\":" + json_helper::quote(r[i][1].as<std::string>()) + ",\"price\":" + r[i][2].as<std::string>() + "}";
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"order_by\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() + ",\"success\":false,\"error\":" + json_helper::quote(err) + ",\"lab_message\":\"ORDER BY concatenation (training). Use whitelist or parameterized patterns.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- 7. Limit SQLi training: concatenate n into LIMIT (unsafe) ---
    CROW_ROUTE(app, "/lab/sqli/limit")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* nParam = req.url_params.get("n");
        std::string nStr = nParam ? nParam : "10";
        if (nStr.size() > 20) nStr = nStr.substr(0, 20);

        try {
            auto& db = Database::instance().getLabConnection();
            pqxx::work txn(db);
            txn.exec("SET statement_timeout = 5000");
            // UNSAFE: concatenating user input into LIMIT clause.
            std::string sql = "SELECT p.id, p.name FROM products p ORDER BY p.id LIMIT " + nStr;
            auto r = txn.exec(sql);
            txn.commit();
            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += "{\"id\":" + r[i][0].as<std::string>() + ",\"name\":" + json_helper::quote(r[i][1].as<std::string>()) + "}";
            }
            arr += "]";
            std::string body = "{" + lab_warning_prefix() + ",\"data\":" + arr + ",\"sqli_type\":\"limit\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
            return crow::response(200, "application/json", body);
        } catch (std::exception& e) {
            std::string err = std::string(e.what());
            std::string body = "{" + lab_warning_prefix() + ",\"success\":false,\"error\":" + json_helper::quote(err) + ",\"lab_message\":\"LIMIT concatenation (training). Use strict integer parsing.\"}";
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 500);
            return crow::response(500, "application/json", body);
        }
    });

    // --- Validation demo: analyze input (bad vs correct validation examples) ---
    CROW_ROUTE(app, "/lab/validation_demo")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        const char* inputParam = req.url_params.get("input");
        std::string input = inputParam ? inputParam : "";
        if (input.size() > 2000) input = input.substr(0, 2000);  // Cap for response size

        auto result = lab::validation_demo::analyze_input(input);

        std::string body = "{" + lab_warning_prefix() +
            ",\"input_length\":" + std::to_string(result.input_length) +
            ",\"is_dangerous\":" + (result.is_dangerous ? "true" : "false") +
            ",\"reason\":" + json_helper::quote(result.reason) +
            ",\"how_to_fix\":" + json_helper::quote(result.how_to_fix) + "}";
        lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
        return crow::response(200, "application/json", body);
    });

    // --- Educational (no DB) endpoints ---
    CROW_ROUTE(app, "/api/lab/sql_injection_explained")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        std::string data = R"json({"title":"SQL Injection - Educational Overview","description":"SQL injection occurs when user input is concatenated directly into SQL queries instead of using parameterized queries.","vulnerable_example":"SELECT * FROM users WHERE email = ' + user_input + ","safe_example":"SELECT * FROM users WHERE email = $1 (with parameter binding)","explanation":"When using string concatenation, an attacker could pass: admin'-- to bypass authentication. Prepared statements prevent this by treating input as data, not code.","prevention":["Use prepared statements (libpqxx uses $1, $2)","Never concatenate user input into SQL","Validate and sanitize input"]})json";
        lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
        return crow::response(200, "application/json", data);
    });

    CROW_ROUTE(app, "/api/lab/memory_safety_explained")
        .methods("GET"_method)
    ([lab_mode_enabled](const crow::request& req) {
        if (auto r = lab::guard(req, lab_mode_enabled)) {
            lab::telemetry::log_request(req.url, build_params_redacted(req), "none", r->code);
            return *r;
        }
        std::string data = R"json({"title":"Memory Safety - Buffer Overflow and ASan","buffer_overflow":{"description":"A buffer overflow occurs when data is written beyond allocated memory.","example":"char buf[4]; strcpy(buf, \"Hello\");","consequences":"Can overwrite return addresses and cause crashes"},"address_sanitizer":{"description":"AddressSanitizer detects memory errors at runtime.","detects":["Buffer overflows","Use-after-free"],"usage":"Compile with -fsanitize=address -g"},"safe_alternatives":["Use std::string","Use std::vector","Avoid strcpy"]})json";
        lab::telemetry::log_request(req.url, build_params_redacted(req), "none", 200);
        return crow::response(200, "application/json", data);
    });
}

}
