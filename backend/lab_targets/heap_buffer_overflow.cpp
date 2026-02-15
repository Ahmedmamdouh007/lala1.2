/**
 * Memory lab: heap buffer overflow (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Run with ASan: AddressSanitizer reports heap-buffer-overflow.
 */

#include <cstring>
#include <iostream>

int main() {
    std::cout << "Memory lab: heap buffer overflow. Writing past a 4-byte heap buffer.\n";
    char* p = new char[4];
    (void)strcpy(p, "Hello World");
    std::cout << p << std::endl;
    delete[] p;
    return 0;
}
