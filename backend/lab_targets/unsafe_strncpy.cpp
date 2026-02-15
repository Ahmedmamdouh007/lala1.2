/**
 * Memory lab: unsafe strncpy (no null terminator when source length >= size).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * strncpy(dst, src, sizeof(dst)) with long src does not null-terminate; later use as string -> read past buffer.
 * Run with ASan: heap-buffer-overflow (read) or stack-buffer-overflow depending on usage.
 */

#include <cstring>
#include <iostream>

int main() {
    std::cout << "Memory lab: unsafe strncpy. No null terminator when source >= size.\n";
    char buf[8];
    const char* long_src = "0123456789ABCDEF";  // 16 chars
    // BUG: strncpy does not null-terminate if strlen(long_src) >= sizeof(buf); buf is then used as string
    (void)strncpy(buf, long_src, sizeof(buf));
    std::cout << buf << std::endl;  // May read past buf if implementation assumes null terminator
    return 0;
}
