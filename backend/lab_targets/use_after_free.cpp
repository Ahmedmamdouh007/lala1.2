/**
 * Memory lab: use-after-free (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Run with ASan: AddressSanitizer reports use-after-free.
 */

#include <iostream>

int main() {
    std::cout << "Memory lab: use-after-free. Using a pointer after delete.\n";
    int* p = new int(42);
    delete p;
    *p = 99;  // Use after free; ASan catches it
    std::cout << *p << std::endl;
    return 0;
}
