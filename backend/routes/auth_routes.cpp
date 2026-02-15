#include "crow.h"
#include "../db/connection.h"
#include "../models/User.h"
#include "../utils/response_helper.h"
#include "../utils/json_helper.h"
#include <pqxx/pqxx>
#include <regex>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace auth_routes {

std::string sha256_hash(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(str.c_str()), str.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool is_valid_email(const std::string& email) {
    std::regex e(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, e);
}

void register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/auth/register")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body) {
                return crow::response(400, response_helper::error_json("Invalid JSON"));
            }
            if (!body["email"] || !body["password"] || !body["name"]) {
                return crow::response(400, response_helper::error_json("Missing email, password, or name"));
            }
            std::string email = body["email"].s();
            std::string password = body["password"].s();
            std::string name = body["name"].s();

            if (email.empty() || password.empty() || name.empty()) {
                return crow::response(400, response_helper::error_json("All fields required"));
            }
            if (!is_valid_email(email)) {
                return crow::response(400, response_helper::error_json("Invalid email format"));
            }
            if (password.size() < 6) {
                return crow::response(400, response_helper::error_json("Password must be at least 6 characters"));
            }

            std::string hash = sha256_hash(password);
            auto& db = Database::instance().getConnection();

            pqxx::work txn(db);
            auto r = txn.exec_params(
                "INSERT INTO users (email, password_hash, name) VALUES ($1, $2, $3) RETURNING id, email, name, created_at",
                email, hash, name
            );
            txn.commit();

            int id = r[0][0].as<int>();
            std::string created_at = r[0][3].as<std::string>();

            std::string data = "{\"user\":{\"id\":" + std::to_string(id) +
                ",\"email\":\"" + json_helper::escape(email) +
                "\",\"name\":\"" + json_helper::escape(name) +
                "\",\"created_at\":\"" + json_helper::escape(created_at) + "\"}}";
            return crow::response(201, response_helper::success_json(data));
        } catch (pqxx::unique_violation&) {
            return crow::response(409, response_helper::error_json("Email already registered"));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/auth/login")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body["email"] || !body["password"]) {
                return crow::response(400, response_helper::error_json("Missing email or password"));
            }
            std::string email = body["email"].s();
            std::string password = body["password"].s();
            std::string hash = sha256_hash(password);

            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto r = txn.exec_params(
                "SELECT id, email, name, created_at FROM users WHERE email = $1 AND password_hash = $2",
                email, hash
            );
            txn.commit();

            if (r.empty()) {
                return crow::response(401, response_helper::error_json("Invalid email or password"));
            }

            int id = r[0][0].as<int>();
            std::string name = r[0][2].as<std::string>();
            std::string created_at = r[0][3].as<std::string>();

            std::string data = "{\"user\":{\"id\":" + std::to_string(id) +
                ",\"email\":\"" + json_helper::escape(email) +
                "\",\"name\":\"" + json_helper::escape(name) +
                "\",\"created_at\":\"" + json_helper::escape(created_at) + "\"}}";
            return crow::response(200, response_helper::success_json(data));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });
}

}
