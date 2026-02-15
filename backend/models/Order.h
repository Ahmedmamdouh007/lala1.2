#pragma once

#include <string>
#include <vector>

struct OrderItem {
    int product_id;
    std::string product_name;
    int quantity;
    double price_at_purchase;
};

struct Order {
    int id;
    int user_id;
    double total;
    std::string status;
    std::string created_at;
    std::vector<OrderItem> items;
};
