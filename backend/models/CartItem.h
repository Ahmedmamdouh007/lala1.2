#pragma once

#include <string>

struct CartItem {
    int id;
    int user_id;
    int product_id;
    int quantity;
    std::string product_name;
    double price;
    std::string image_url;
};
