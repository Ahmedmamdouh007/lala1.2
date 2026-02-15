#include "crow.h"
#include "../db/connection.h"
#include "../models/Order.h"
#include "../utils/response_helper.h"
#include "../utils/json_helper.h"
#include <pqxx/pqxx>

namespace order_routes {

void register_routes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/api/orders/create")
        .methods("POST"_method)
    ([](const crow::request& req) {
        try {
            auto body = crow::json::load(req.body);
            if (!body || !body["user_id"] || !body["items"]) {
                return crow::response(400, response_helper::error_json("Missing user_id or items"));
            }
            int userId = body["user_id"].i();
            auto& items = body["items"];

            if (items.size() == 0) {
                return crow::response(400, response_helper::error_json("No items in order"));
            }

            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);

            double total = 0;
            for (size_t i = 0; i < items.size(); i++) {
                auto item = items[i];
                int productId = item["product_id"].i();
                int qty = item["quantity"].i();
                if (qty < 1) continue;

                auto pr = txn.exec_params("SELECT price, stock FROM products WHERE id = $1", productId);
                if (pr.empty()) {
                    txn.abort();
                    return crow::response(400, response_helper::error_json("Product not found: " + std::to_string(productId)));
                }
                double price = pr[0][0].as<double>();
                int stock = pr[0][1].as<int>();
                if (qty > stock) {
                    txn.abort();
                    return crow::response(400, response_helper::error_json("Insufficient stock for product " + std::to_string(productId)));
                }
                total += price * qty;
            }

            auto orderR = txn.exec_params(
                "INSERT INTO orders (user_id, total, status) VALUES ($1, $2, 'pending') RETURNING id, created_at",
                userId, total
            );
            int orderId = orderR[0][0].as<int>();

            for (size_t i = 0; i < items.size(); i++) {
                auto item = items[i];
                int productId = item["product_id"].i();
                int qty = item["quantity"].i();
                if (qty < 1) continue;

                auto pr = txn.exec_params("SELECT price, name FROM products WHERE id = $1", productId);
                double price = pr[0][0].as<double>();

                txn.exec_params(
                    "INSERT INTO order_items (order_id, product_id, quantity, price_at_purchase) VALUES ($1, $2, $3, $4)",
                    orderId, productId, qty, price
                );
                txn.exec_params(
                    "UPDATE products SET stock = stock - $1 WHERE id = $2",
                    qty, productId
                );
            }

            txn.exec_params("DELETE FROM cart_items WHERE user_id = $1", userId);
            txn.commit();

            std::string data = "{\"order_id\":" + std::to_string(orderId) + ",\"total\":" + json_helper::double_to_str(total) + "}";
            return crow::response(201, response_helper::success_json(data));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });

    CROW_ROUTE(app, "/api/orders/<int>")
        .methods("GET"_method)
    ([](int userId) {
        try {
            auto& db = Database::instance().getConnection();
            pqxx::work txn(db);
            auto orders = txn.exec_params(
                "SELECT id, user_id, total, status, created_at FROM orders WHERE user_id = $1 ORDER BY created_at DESC",
                userId
            );
            txn.commit();

            std::string arr = "[";
            for (size_t oi = 0; oi < orders.size(); oi++) {
                if (oi > 0) arr += ",";
                int orderId = orders[oi][0].as<int>();
                double total = orders[oi][2].as<double>();
                std::string status = orders[oi][3].as<std::string>();
                std::string created = orders[oi][4].as<std::string>();

                pqxx::work txn2(db);
                auto items = txn2.exec_params(
                    "SELECT oi.product_id, p.name, oi.quantity, oi.price_at_purchase "
                    "FROM order_items oi JOIN products p ON oi.product_id = p.id WHERE oi.order_id = $1",
                    orderId
                );
                txn2.commit();

                std::string itemsArr = "[";
                for (size_t ii = 0; ii < items.size(); ii++) {
                    if (ii > 0) itemsArr += ",";
                    itemsArr += "{\"product_id\":" + std::to_string(items[ii][0].as<int>()) +
                        ",\"product_name\":" + json_helper::quote(items[ii][1].as<std::string>()) +
                        ",\"quantity\":" + std::to_string(items[ii][2].as<int>()) +
                        ",\"price_at_purchase\":" + json_helper::double_to_str(items[ii][3].as<double>()) + "}";
                }
                itemsArr += "]";

                arr += "{\"id\":" + std::to_string(orderId) +
                    ",\"user_id\":" + std::to_string(userId) +
                    ",\"total\":" + json_helper::double_to_str(total) +
                    ",\"status\":\"" + json_helper::escape(status) + "\"" +
                    ",\"created_at\":\"" + json_helper::escape(created) + "\"" +
                    ",\"items\":" + itemsArr + "}";
            }
            arr += "]";
            return crow::response(200, response_helper::success_json(arr));
        } catch (std::exception& e) {
            return crow::response(500, response_helper::error_json(std::string("Error: ") + e.what()));
        }
    });
}

}
