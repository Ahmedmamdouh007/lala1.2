#pragma once

#include <pqxx/pqxx>
#include <memory>
#include <string>

struct DbConfig {
    std::string host;
    int port;
    std::string dbname;
    std::string user;
    std::string password;
    std::string lab_user;
    std::string lab_password;
};

class Database {
public:
    static Database& instance();
    void loadConfig(const std::string& configPath);
    /// Main app connection (app_user). Use for normal routes.
    pqxx::connection& getConnection();
    /// Lab connection (lab_readonly). Use for /lab routes. SELECT only on products/categories.
    pqxx::connection& getLabConnection();
    bool isSecurityLabMode() const { return security_lab_mode_; }
    void setSecurityLabMode(bool v) { security_lab_mode_ = v; }

private:
    Database() = default;
    std::unique_ptr<pqxx::connection> conn_;
    std::unique_ptr<pqxx::connection> lab_conn_;
    DbConfig config_;
    bool security_lab_mode_ = false;
};
