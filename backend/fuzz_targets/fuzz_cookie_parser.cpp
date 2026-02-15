/**
 * Fuzz target: internal Cookie header parser (name=value; name2=value2).
 * Fuzzes parse_cookie_header() - not HTTP layer.
 * Build: -fsanitize=fuzzer,address,undefined
 */

#include <cstdint>
#include <cstdlib>
#include <string>
#include <map>
#include <cctype>

namespace {

/// Parse Cookie header value into name -> value map. No unescaping.
std::map<std::string, std::string> parse_cookie_header(const char* data, size_t size) {
    std::map<std::string, std::string> out;
    std::string name, value;
    enum { Name, Value } state = Name;
    for (size_t i = 0; i < size; ++i) {
        char c = data[i];
        if (state == Name) {
            if (c == '=' && !name.empty()) {
                state = Value;
                value.clear();
            } else if (std::isspace(static_cast<unsigned char>(c)))
                continue;
            else if (c == ';') {
                if (!name.empty()) out[std::move(name)] = "";
                name.clear();
            } else
                name.push_back(c);
        } else {
            if (c == ';') {
                if (!name.empty()) out[std::move(name)] = std::move(value);
                name.clear();
                value.clear();
                state = Name;
            } else
                value.push_back(c);
        }
    }
    if (!name.empty()) out[std::move(name)] = std::move(value);
    return out;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    auto cookies = parse_cookie_header(reinterpret_cast<const char*>(Data), Size);
    (void)cookies;
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
