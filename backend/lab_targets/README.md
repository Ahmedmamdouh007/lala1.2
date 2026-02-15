# Memory lab targets

Standalone C++ programs that trigger specific memory bugs for **AddressSanitizer (ASan)** training. Each program prints a short explanation, then crashes deterministically.

## Programs

| Binary | Bug type |
|--------|----------|
| `stack_buffer_overflow` | Stack buffer overflow (`strcpy` past end) |
| `heap_buffer_overflow` | Heap buffer overflow |
| `use_after_free` | Use after free |
| `double_free` | Double free |
| `out_of_bounds_read` | Out-of-bounds read |
| `null_deref` | Null pointer dereference |
| `integer_overflow_to_alloc` | Undersized allocation (simulated overflow) then OOB write |
| `format_string_bug` | Format string → buffer overflow (`sprintf` into small buffer) |

## Build profiles (CMake)

From **backend/build**:

**Normal debug** (no sanitizer; will crash but no ASan report):

```bash
cmake -DBUILD_MEMORY_LABS=ON ..
cmake --build .
```

**ASan debug** (AddressSanitizer; deterministic ASan report):

```bash
cmake -DBUILD_MEMORY_LABS=ON -DMEMORY_LABS_ASAN=ON ..
cmake --build .
```

All memory lab binaries are built with `-g -fno-omit-frame-pointer` for readable stack traces. With `MEMORY_LABS_ASAN=ON`, `-fsanitize=address` is added so ASan catches the bugs.

## Run

```bash
./stack_buffer_overflow
./heap_buffer_overflow
# ... etc.
```

Run with the ASan build to see the sanitizer report for each bug type.
