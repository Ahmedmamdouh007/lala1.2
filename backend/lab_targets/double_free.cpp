/**
 * Memory lab: double free (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Run with ASan: AddressSanitizer reports double-free.
 */

#include <iostream>

int main() {
    std::cout << "Memory lab: double free. Calling delete twice on the same pointer.\n";
    int* p = new int(1);
    delete p;
    delete p;  // Double free; ASan catches it
    return 0;
}
