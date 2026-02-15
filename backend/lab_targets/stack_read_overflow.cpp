/**
 * Memory lab: stack buffer read overflow (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Fixed OOB read from stack-allocated array. Run with ASan: stack-buffer-overflow (read).
 */

#include <iostream>

int main() {
    std::cout << "Memory lab: stack read overflow. Reading past end of stack buffer.\n";
    int stack_arr[4] = { 1, 2, 3, 4 };
    volatile int x = stack_arr[10];  // OOB read; ASan catches it
    (void)x;
    return 0;
}
