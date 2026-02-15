/**
 * Fuzz target: internal cart payload parsing (JSON body as in cart routes).
 * Fuzzes crow::json::load and cart-like field access - not HTTP endpoints.
 * Build: -fsanitize=fuzzer,address,undefined
 */

#include "crow/json.h"
#include <cstdint>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    auto body = crow::json::load(reinterpret_cast<const char*>(Data), Size);
    if (!body || body.t() != crow::json::type::Object) return 0;
    try {
        if (!body["user_id"]) return 0;
        if (!body["product_id"]) return 0;
        int userId = body["user_id"].i();
        int productId = body["product_id"].i();
        int quantity = body["quantity"] ? static_cast<int>(body["quantity"].i()) : 1;
        (void)userId;
        (void)productId;
        (void)quantity;
    } catch (...) {}
    return 0;
}

#ifndef FUZZING_BUILD_MODE
#include <fstream>
#include <iostream>
#include <vector>
int main(int argc, char** argv) {
    std::string path = argc >= 2 ? argv[1] : "default_corpus.txt";
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cerr << "Open failed: " << path << "\n"; return 1; }
    f.seekg(0, std::ios::end);
    size_t len = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(len);
    f.read(reinterpret_cast<char*>(buf.data()), len);
    return LLVMFuzzerTestOneInput(buf.data(), buf.size());
}
#endif
