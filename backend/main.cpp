#include "crow.h"
#include "db/connection.h"
#include "routes/auth_routes.h"
#include "routes/product_routes.h"
#include "routes/cart_routes.h"
#include "routes/order_routes.h"
#ifdef ENABLE_LABS
#include "routes/lab_routes.h"
#include "lab_services/tcp_lab_server.h"
#endif
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {

void print_lab_mode_banner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                                                                              ║\n";
    std::cout << "  ║   ***  LAB_MODE=true  ***   SECURITY LAB ENABLED   ***  DO NOT DEPLOY  ***   ║\n";
    std::cout << "  ║                                                                              ║\n";
    std::cout << "  ║   Lab routes accept requests from 127.0.0.1 only.                            ║\n";
    std::cout << "  ║   /lab/sqli/search  /lab/sqli/product  /lab/sqli/error_based               ║\n";
    std::cout << "  ║   /lab/sqli/boolean_based  /lab/sqli/time_based  /lab/sqli/union_based   ║\n";
    std::cout << "  ║   /lab/sqli/auth_bypass  /api/lab/sql_injection_explained  (memory_safety) ║\n";
    std::cout << "  ║   TCP lab service: 127.0.0.1:9001 (LEN(2)+DATA -> OK/ERR)                    ║\n";
    std::cout << "  ║                                                                              ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "config/db_config.json";
    for (int i = 1; i < argc - 1; i++) {
        if (std::string(argv[i]) == "--config") {
            configPath = argv[i + 1];
            break;
        }
    }

    const char* labModeEnv = std::getenv("LAB_MODE");
    bool labMode = (labModeEnv && (std::string(labModeEnv) == "true" || std::string(labModeEnv) == "1"));

    try {
        Database::instance().loadConfig(configPath);
        Database::instance().setSecurityLabMode(labMode);
        std::cout << "Database connected. LAB_MODE=" << (labMode ? "true" : "false") << std::endl;
    } catch (std::exception& e) {
        std::cerr << "Failed to connect to database: " << e.what() << std::endl;
        return 1;
    }

    crow::SimpleApp app;

    app.loglevel(crow::LogLevel::Warning);

    auth_routes::register_routes(app);
    product_routes::register_routes(app);
    cart_routes::register_routes(app);
    order_routes::register_routes(app);

#ifdef ENABLE_LABS
    lab_routes::register_routes(app, labMode);
    if (labMode) {
        print_lab_mode_banner();
        std::thread tcp_lab(run_tcp_lab_server);
        tcp_lab.detach();
    }
#endif

    app.port(8080).multithreaded().run();
    return 0;
}
