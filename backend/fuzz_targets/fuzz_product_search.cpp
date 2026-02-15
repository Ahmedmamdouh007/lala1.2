/**
 * Fuzz target: Search term processing used by product search.
 * Exercises: term length limit, building "%term%" pattern, string handling.
 * Build with: -fsanitize=fuzzer,address
 *
 * Seed corpus: backend/fuzz_corpus/product_search/ (contains CRASHME to trigger lab crash).
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>

namespace {

constexpr size_t MAX_SEARCH_TERM_LEN = 500;
constexpr const char LAB_CRASH_TRIGGER[] = "CRASHME";  // Corpus seed triggers this for teaching

// Mirror product search: get search term, limit length, build LIKE pattern "%term%".
std::string process_search_term(const std::string& raw) {
    std::string term = raw;
    if (term.size() > MAX_SEARCH_TERM_LEN)
        term.resize(MAX_SEARCH_TERM_LEN);
    return "%" + term + "%";
}

// Additional processing: trim leading/trailing spaces (API may do this).
std::string process_search_term_trimmed(std::string raw) {
    while (!raw.empty() && raw.back() == ' ') raw.pop_back();
    size_t i = 0;
    while (i < raw.size() && raw[i] == ' ') ++i;
    raw.erase(0, i);
    if (raw.size() > MAX_SEARCH_TERM_LEN)
        raw.resize(MAX_SEARCH_TERM_LEN);
    return "%" + raw + "%";
}

// Intentional lab bug: when input starts with CRASHME, copy into tiny buffer (for corpus demo).
// Corpus file fuzz_corpus/product_search/crash_trigger contains CRASHME so fuzzer finds crash quickly.
static void lab_crash_if_triggered(const uint8_t* Data, size_t Size) {
    const size_t trigger_len = sizeof(LAB_CRASH_TRIGGER) - 1;
    if (Size >= trigger_len && std::memcmp(Data, LAB_CRASH_TRIGGER, trigger_len) == 0) {
        char small[4];
        size_t copy_len = Size > 20 ? 20 : Size;  // Deliberate overflow when Size > 4
        std::memcpy(small, Data, copy_len);       // Crash: writes past small[4]
        (void)small;
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    if (Size == 0) return 0;
    std::string input(reinterpret_cast<const char*>(Data), Size);

    lab_crash_if_triggered(Data, Size);
    (void)process_search_term(input);
    (void)process_search_term_trimmed(input);
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
