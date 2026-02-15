#include "crow.h"
#include "../db/connection.h"
#include "../models/CartItem.h"
#include "../utils/response_helper.h"
#include "../utils/json_helper.h"
#include <pqxx/pqxx>

namespace cart_routes {

std::string cart_item_to_json(const pqxx::row& row) {
    int id = row[0].as<int>();
    int user_id = row[1].as<int>();
    int product_id = row[2].as<int>();
    int qty = row[3].as<int>();
    std::string pname = row[4].as<std::string>();
    double price = row[5].as<double>();
    std::string img = row[6].is_null() ? "" : row[6].as<std::string>();

    return "{\"id\":" + std::to_string(id) +
        ",\"user_id\":" + std::to_string(user_id) +
        ",\"product_id\":" + std::to_string(product_id) +
        ",\"quantity\":" + std::to_string(qty) +
        ",\"product_name\":" + json_helper::quote(pname) +
        ",\"price\":" + json_helper::double_to_str(price) +
        ",\"image_url\":" + json_helper::quote(img) + "}";
}

void register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/cart/<int>")
        .methods("GET"_method)
    ([](int userId) {
        try {
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto r = txn.exec_params(
                "SELECT ci.id, ci.user_id, ci.product_id, ci.quantity, p.name, p.price, p.image_url "
                "FROM cart_items ci JOIN products p ON ci.product_id = p.id WHERE ci.user_id = $1",
                userId
            );
            txn.commit();

            std::string arr = "[";
            for (size_t i = 0; i < r.size(); i++) {
                if (i > 0) arr += ",";
                arr += cart_item_to_json(r[i]);
            }
            arr += "]";
            return crow::response(200, response_helper::success_json(arr));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/cart/add")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body["user_id"] || !body["product_id"]) {
                return crow::response(400, response_helper::error_json("Missing user_id or product_id"));
            }
            int userId = body["user_id"].i();
            int productId = body["product_id"].i();
            int quantity = body["quantity"] ? static_cast<int>(body["quantity"].i()) : 1;
            if (quantity < 1) quantity = 1;

            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            txn.exec_params(
                "INSERT INTO cart_items (user_id, product_id, quantity) VALUES ($1, $2, $3) "
                "ON CONFLICT (user_id, product_id) DO UPDATE SET quantity = cart_items.quantity + EXCLUDED.quantity",
                userId, productId, quantity
            );
            txn.commit();

            return crow::response(201, response_helper::success_message("Item added to cart"));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/cart/remove")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body["user_id"] || !body["product_id"]) {
                return crow::response(400, response_helper::error_json("Missing user_id or product_id"));
            }
            int userId = body["user_id"].i();
            int productId = body["product_id"].i();

            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            txn.exec_params(
                "DELETE FROM cart_items WHERE user_id = $1 AND product_id = $2",
                userId, productId
            );
            txn.commit();

            return crow::response(200, response_helper::success_message("Item removed from cart"));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/cart/update_quantity")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body["user_id"] || !body["product_id"] || !body["quantity"]) {
                return crow::response(400, response_helper::error_json("Missing user_id, product_id, or quantity"));
            }
            int userId = body["user_id"].i();
            int productId = body["product_id"].i();
            int quantity = body["quantity"].i();
            if (quantity < 1) {
                return crow::response(400, response_helper::error_json("Quantity must be at least 1"));
            }

            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            txn.exec_params(
                "UPDATE cart_items SET quantity = $1 WHERE user_id = $2 AND product_id = $3",
                quantity, userId, productId
            );
            txn.commit();

            return crow::response(200, response_helper::success_message("Cart updated"));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });
}

}
