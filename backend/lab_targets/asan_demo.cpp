/**
 * ASan Demo - Educational purposes only.
 * This file is NOT connected to the web server.
 * It intentionally triggers one deterministic buffer overflow for learning.
 *
 * Build with ASan and symbols (required for readable stack traces):
 *   clang++ -fsanitize=address -g -fno-omit-frame-pointer -o asan_demo asan_demo.cpp
 *   g++ -fsanitize=address -g -fno-omit-frame-pointer -o asan_demo asan_demo.cpp
 *
 * Run: ./asan_demo
 * Expected: Same heap-buffer-overflow report every run (deterministic).
 */

#include <cstring>
#include <iostream>

// Single, deterministic bug: fixed buffer and fixed overflow so ASan report is identical every run.
static void heap_buffer_overflow_demo() {
    char* buf = new char[4];
    (void)strcpy(buf, "Hello World");  // Writes beyond buffer - ASan catches this every time
    std::cout << buf << std::endl;
    delete[] buf;
}

int main() {
    std::cout << "ASan Demo - deterministic heap buffer overflow (educational)" << std::endl;
    heap_buffer_overflow_demo();  // Always triggers the same ASan report
    return 0;
}
