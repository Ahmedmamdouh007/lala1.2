#include "crow.h"
#include "../db/connection.h"
#include "../models/Product.h"
#include "../utils/response_helper.h"
#include "../utils/json_helper.h"
#include <pqxx/pqxx>

namespace product_routes {

std::string product_to_json(const pqxx::row& row) {
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

void register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/products")
        .methods("GET"_method)
    ([](const crow::request&) {
        try {
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto r = txn.exec(
                "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id ORDER BY p.id"
            );
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_to_json(r[i]);
            }
            arr += "]";
            return crow::response(200, response_helper::success_json(arr));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/products/<int>")
        .methods("GET"_method)
    ([](int id) {
        try {
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto r = txn.exec_params(
                "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id WHERE p.id = $1",
                id
            );
            txn.commit();

            if (r.empty()) {
                return crow::response(404, response_helper::error_json("Product not found"));
            }
            return crow::response(200, response_helper::success_json(product_to_json(r[0])));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/products/category/<string>")
        .methods("GET"_method)
    ([](const std::string& categoryName) {
        try {
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto r = txn.exec_params(
                "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE LOWER(c.name) = LOWER($1) ORDER BY p.id",
                categoryName
            );
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_to_json(r[i]);
            }
            arr += "]";
            return crow::response(200, response_helper::success_json(arr));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/products/search")
        .methods("GET"_method)
    ([](const crow::request& req) {
        try {
            std::string q = req.url_params.get("q") ? req.url_params.get("q") : "";
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            std::string search = "%" + q + "%";
            auto r = txn.exec_params(
                "SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock, "
                "c.name as cat_name, p.created_at FROM products p "
                "LEFT JOIN categories c ON p.category_id = c.id "
                "WHERE p.name ILIKE $1 OR p.description ILIKE $1 ORDER BY p.id",
                search
            );
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += product_to_json(r[i]);
            }
            arr += "]";
            return crow::response(200, response_helper::success_json(arr));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });
}

}
