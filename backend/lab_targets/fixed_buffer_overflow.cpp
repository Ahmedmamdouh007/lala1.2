/**
 * Memory lab: fixed-size buffer overflow via copy (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Copy "long string" into 6-byte stack buffer with memcpy; no bounds check. Run with ASan: stack-buffer-overflow.
 */

#include <cstring>
#include <iostream>

int main() {
    std::cout << "Memory lab: fixed buffer overflow. memcpy long string into 6-byte buffer.\n";
    char buf[6];
    const char* src = "overflow_me";
    (void)memcpy(buf, src, strlen(src) + 1);  // Copies 12 bytes into 6-byte buffer; ASan catches it
    std::cout << buf << std::endl;
    return 0;
}
