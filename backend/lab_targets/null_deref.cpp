/**
 * Memory lab: null pointer dereference (deterministic).
 * Build: see backend/CMakeLists.txt (memory lab targets).
 * Crashes on *nullptr; ASan can report it (or OS SIGSEGV). Deterministic.
 */

#include <iostream>

int main() {
    std::cout << "Memory lab: null pointer dereference. Dereferencing nullptr.\n";
    int* p = nullptr;
    *p = 42;  // Null deref; deterministic crash
    return 0;
}
