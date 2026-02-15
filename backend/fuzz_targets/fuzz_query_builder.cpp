/**
 * Fuzz target: Input validation / query-building logic used by the API.
 * Exercises: ID string sanitization, search term length/validation.
 * Build with: -fsanitize=fuzzer,address
 */

#include <cstdint>
#include <cstdlib>
#include <string>
#include <cctype>

namespace {

// Mirror API validation: sanitize string to numeric-only for ID (max 10 digits).
std::string sanitize_id_string(const std::string& raw) {
    std::string out;
    out.reserve(std::min(raw.size(), size_t(10)));
    for (char c : raw) {
        if (std::isdigit(static_cast<unsigned char>(c)) && out.size() < 10)
            out += c;
    }
    return out;
}

// Mirror API: limit search term length and strip control characters.
void validate_search_term_length(std::string& s, size_t max_len) {
    if (s.size() > max_len)
        s.resize(max_len);
    for (size_t i = 0; i < s.size(); ) {
        unsigned char u = static_cast<unsigned char>(s[i]);
        if (u < 32 && u != '\t') {
            s.erase(i, 1);
        } else {
            ++i;
        }
    }
}

// Build a LIKE pattern string (as used in product search); apply length limit.
std::string build_search_like_pattern(const std::string& term, size_t max_len) {
    std::string t = term;
    validate_search_term_length(t, max_len);
    return "%" + t + "%";
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    std::string input(reinterpret_cast<const char*>(Data), Size);

    (void)sanitize_id_string(input);
    std::string term = input;
    validate_search_term_length(term, 500);
    (void)build_search_like_pattern(input, 500);
    return 0;
}

#ifndef FUZZING_BUILD_MODE
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
