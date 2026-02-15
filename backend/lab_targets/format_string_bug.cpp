/**
 * Memory lab: format string bug (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * sprintf into small buffer with format that produces long output -> stack-buffer-overflow.
 * Run with ASan: stack-buffer-overflow.
 */

#include <cstdio>
#include <iostream>

int main() {
    std::cout << "Memory lab: format string bug. sprintf long output into small buffer.\n";
    char buf[4];
    // Simulate: sprintf(buf, user_controlled_format, ...) with no size limit.
    (void)sprintf(buf, "%s", "overflow");  // Writes past buf; ASan catches it
    std::cout << buf << std::endl;
    return 0;
}
