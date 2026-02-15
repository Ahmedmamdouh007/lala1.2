#pragma once

#include <string>

struct Product {
    int id;
    int category_id;
    std::string name;
    std::string description;
    double price;
    std::string image_url;
    int stock;
    std::string category_name;
    std::string created_at;
};
