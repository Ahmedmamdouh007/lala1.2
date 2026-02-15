/**
 * Fuzz target: JSON parsing used by the API (crow::json::load).
 * Build with: -fsanitize=fuzzer,address
 * Run: ./fuzz_json_parser [corpus_dir]   or   ./fuzz_json_parser -runs=100 ./corpus
 */

#include "crow/json.h"
#include <cstdint>
#include <cstdlib>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    auto body = crow::json::load(reinterpret_cast<const char*>(Data), Size);
    (void)body;  // exercise parser; may be invalid
    return 0;
}

#ifndef FUZZING_BUILD_MODE
// Standalone driver when not linked with libFuzzer (e.g. reproduce crash from file)
#include <fstream>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    std::string path = "default_corpus.txt";
    if (argc >= 2) path = argv[1];
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        std::cerr << "Open failed: " << path << std::endl;
        return 1;
    }
    f.seekg(0, std::ios::end);
    size_t len = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(len);
    f.read(reinterpret_cast<char*>(buf.data()), len);
    f.close();
    return LLVMFuzzerTestOneInput(buf.data(), buf.size());
}
#endif
