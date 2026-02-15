/**
 * Memory lab: out-of-bounds read (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Run with ASan: AddressSanitizer reports heap-buffer-overflow (read).
 */

#include <iostream>

int main() {
    std::cout << "Memory lab: out-of-bounds read. Reading past end of heap buffer.\n";
    int* arr = new int[5];
    for (int i = 0; i < 5; ++i) arr[i] = i;
    volatile int x = arr[10];  // OOB read; ASan catches it
    (void)x;
    delete[] arr;
    return 0;
}
