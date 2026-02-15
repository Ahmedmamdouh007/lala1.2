/**
 * Fuzz target: internal search term processing (no HTTP).
 * Fuzzes length limit, trim, and "%term%" pattern building.
 * Build: -fsanitize=fuzzer,address,undefined
 */

#include <cstdint>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

namespace {

constexpr size_t MAX_SEARCH_TERM_LEN = 500;

std::string process_search_term(const std::string& raw) {
    std::string term = raw;
    if (term.size() > MAX_SEARCH_TERM_LEN)
        term.resize(MAX_SEARCH_TERM_LEN);
    return "%" + term + "%";
}

void trim_leading_trailing(std::string& s) {
    while (!s.empty() && s.back() == ' ') s.pop_back();
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    s.erase(0, i);
}

void strip_control_chars(std::string& s) {
    for (size_t i = 0; i < s.size(); ) {
        unsigned char u = static_cast<unsigned char>(s[i]);
        if (u < 32 && u != '\t')
            s.erase(i, 1);
        else
            ++i;
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    std::string input(reinterpret_cast<const char*>(Data), Size);
    (void)process_search_term(input);
    trim_leading_trailing(input);
    if (input.size() > MAX_SEARCH_TERM_LEN) input.resize(MAX_SEARCH_TERM_LEN);
    (void)("%" + input + "%");
    strip_control_chars(input);
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
