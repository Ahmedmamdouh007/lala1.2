/**
 * Fuzz target: internal JSON body parsing (request body style).
 * Fuzzes crow::json::load and typed accessors - not HTTP endpoints.
 * Build: -fsanitize=fuzzer,address,undefined
 */

#include "crow/json.h"
#include <cstdint>
#include <cstdlib>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    auto body = crow::json::load(reinterpret_cast<const char*>(Data), Size);
    if (!body) return 0;
    try {
        if (body.t() == crow::json::type::Object) {
            for (size_t i = 0; i < body.size(); ++i) (void)body.keys();
            if (body.has("user_id")) (void)body["user_id"].i();
            if (body.has("product_id")) (void)body["product_id"].i();
            if (body.has("quantity")) (void)body["quantity"].i();
            if (body.has("email")) (void)std::string(body["email"].s());
            if (body.has("items") && body["items"].t() == crow::json::type::List) {
                for (size_t j = 0; j < body["items"].size(); ++j)
                    (void)body["items"][static_cast<int>(j)];
            }
        }
        if (body.t() == crow::json::type::List)
            for (size_t i = 0; i < body.size(); ++i) (void)body[static_cast<int>(i)];
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
