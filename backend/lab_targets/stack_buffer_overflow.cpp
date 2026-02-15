/**
 * Memory lab: stack buffer overflow (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Run with ASan: AddressSanitizer reports stack-buffer-overflow.
 */

#include <cstring>
#include <iostream>

int main() {
    std::cout << "Memory lab: stack buffer overflow. Writing past a 4-byte stack buffer.\n";
    char buf[4];
    (void)strcpy(buf, "overflow");  // Deterministic overflow; ASan catches it
    std::cout << buf << std::endl;
    return 0;
}
