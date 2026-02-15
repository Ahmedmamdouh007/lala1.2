#include "connection.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

Database& Database::instance() {
    static Database db;
    return db;
}

void Database::loadConfig(const std::string& configPath) {
    std::ifstream f(configPath);
    if (!f.is_open()) {
        throw std::runtime_error("Cannot open db config: " + configPath);
    }
    std::stringstream buf;
    buf << f.rdbuf();
    std::string content = buf.str();

    // Simple JSON parsing for db_config.json
    auto extract = [&content](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\"";
        size_t pos = content.find(search);
        if (pos == std::string::npos) return "";
        pos = content.find(":", pos);
        if (pos == std::string::npos) return "";
        pos = content.find("\"", pos);
        if (pos == std::string::npos) return "";
        size_t start = pos + 1;
        size_t end = content.find("\"", start);
        if (end == std::string::npos) return "";
        return content.substr(start, end - start);
    };

    config_.host = extract("host");
    if (config_.host.empty()) config_.host = "localhost";
    config_.dbname = extract("dbname");
    if (config_.dbname.empty()) config_.dbname = "lala_store";
    config_.user = extract("user");
    if (config_.user.empty()) config_.user = "postgres";
    config_.password = extract("password");
    if (config_.password.empty()) config_.password = "postgres";
    config_.lab_user = extract("lab_user");
    config_.lab_password = extract("lab_password");

    std::string portStr = extract("port");
    if (!portStr.empty()) {
        config_.port = std::stoi(portStr);
    } else {
        size_t p = content.find("\"port\"");
        if (p != std::string::npos) {
            p = content.find(":", p) + 1;
            while (p < content.size() && (content[p] == ' ' || content[p] == '\t')) p++;
            if (p < content.size() && std::isdigit(static_cast<unsigned char>(content[p]))) {
                config_.port = std::stoi(content.substr(p));
            } else config_.port = 5432;
        } else config_.port = 5432;
    }

    std::string connStr = "host=" + config_.host +
        " port=" + std::to_string(config_.port) +
        " dbname=" + config_.dbname +
        " user=" + config_.user +
        " password=" + config_.password;

    conn_ = std::make_unique<pqxx::connection>(connStr);

    if (!config_.lab_user.empty() && !config_.lab_password.empty()) {
        std::string labConnStr = "host=" + config_.host +
            " port=" + std::to_string(config_.port) +
            " dbname=" + config_.dbname +
            " user=" + config_.lab_user +
            " password=" + config_.lab_password;
        lab_conn_ = std::make_unique<pqxx::connection>(labConnStr);
    }
}

pqxx::connection& Database::getLabConnection() {
    if (!lab_conn_ || !lab_conn_->is_open())
        throw std::runtime_error("Lab database connection not configured (set lab_user and lab_password in db_config.json)");
    return *lab_conn_;
}

pqxx::connection& Database::getConnection() {
    if (!conn_ || !conn_->is_open()) {
        throw std::runtime_error("Database not connected");
    }
    return *conn_;
}
