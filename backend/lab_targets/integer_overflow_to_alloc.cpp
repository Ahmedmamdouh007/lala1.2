/**
 * Memory lab: integer overflow leading to undersized allocation (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Simulates overflow: "large" count * sizeof(int) overflows so alloc is tiny; then OOB write.
 * Run with ASan: heap-buffer-overflow.
 */

#include <iostream>
#include <cstdint>

int main() {
    std::cout << "Memory lab: integer overflow to allocation. Undersized alloc then OOB write.\n";
    // Simulate untrusted count that overflows when multiplied: e.g. 0x80000000 * 4 = 0 in 32-bit
    size_t small_alloc = 2;  // In real bug, (huge_count * sizeof(int)) overflows to small
    int* arr = new int[small_alloc];
    arr[0] = 1;
    arr[100] = 2;  // OOB write; ASan catches it (deterministic)
    delete[] arr;
    return 0;
}
