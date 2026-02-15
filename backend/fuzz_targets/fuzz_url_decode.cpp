/**
 * Fuzz target: internal URL percent-decode (e.g. for query params).
 * Fuzzes url_decode() - not HTTP layer.
 * Build: -fsanitize=fuzzer,address,undefined
 */

#include <cstdint>
#include <cstdlib>
#include <string>
#include <cctype>

namespace {

/// Internal URL percent-decode: %XX -> byte, + -> space.
std::string url_decode(const char* data, size_t size) {
    std::string out;
    out.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '%' && i + 2 < size) {
            unsigned char hi = static_cast<unsigned char>(data[i + 1]);
            unsigned char lo = static_cast<unsigned char>(data[i + 2]);
            if (std::isxdigit(hi) && std::isxdigit(lo)) {
                int h = std::isdigit(hi) ? (hi - '0') : (std::tolower(hi) - 'a' + 10);
                int l = std::isdigit(lo) ? (lo - '0') : (std::tolower(lo) - 'a' + 10);
                out.push_back(static_cast<char>((h * 16 + l) & 0xff));
                i += 2;
                continue;
            }
        }
        if (data[i] == '+') {
            out.push_back(' ');
            continue;
        }
        out.push_back(data[i]);
    }
    return out;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    std::string decoded = url_decode(reinterpret_cast<const char*>(Data), Size);
    (void)decoded;
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
