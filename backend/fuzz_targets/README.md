# Backend fuzz targets

Standalone libFuzzer fuzz targets for the Crow backend. They exercise JSON parsing, input validation, and search term processing **without running the web server**.

## Requirements

- **Clang** (libFuzzer is part of the compiler runtime; GCC is not supported for fuzzing)
- CMake 3.14+

On Debian/Ubuntu: `sudo apt install clang`
On macOS: Xcode Command Line Tools or `brew install llvm` (then use `CXX=clang++` or point CMake to Clang).

## Build commands

From the **backend** directory:

```bash
mkdir -p build && cd build
cmake -DBUILD_FUZZ_TARGETS=ON -DCMAKE_CXX_COMPILER=clang++ ..
cmake --build .
```

This produces **eight** executables in `build/`. All fuzz **internal parsing functions** (not HTTP endpoints). Build uses **`-fsanitize=fuzzer,address,undefined`** (libFuzzer + ASan + UBSan).

| Target | Parsing function fuzzed |
|--------|-------------------------|
| `fuzz_json_parser` | `crow::json::load()` |
| `fuzz_query_builder` | ID sanitization, search-term validation |
| `fuzz_product_search` | Search term processing (+ lab crash seed) |
| `fuzz_url_decode` | Internal URL percent-decode (`%XX`, `+`) |
| `fuzz_json_body` | JSON body + typed accessors (user_id, items, etc.) |
| `fuzz_cart_payload` | Cart JSON payload (user_id, product_id, quantity) |
| `fuzz_search_term` | Search term length/trim/pattern (no HTTP) |
| `fuzz_cookie_parser` | Cookie header parser (`name=value; ...`) |

## Seed corpus (backend/fuzz_corpus/)

At least **10 seed files per target** are provided. One seed in `product_search/` triggers an intentional crash for teaching.

| Directory | Use with |
|-----------|----------|
| `backend/fuzz_corpus/json/` | `fuzz_json_parser` |
| `backend/fuzz_corpus/query_builder/` | `fuzz_query_builder` |
| `backend/fuzz_corpus/product_search/` | `fuzz_product_search` |
| `backend/fuzz_corpus/url_decode/` | `fuzz_url_decode` |
| `backend/fuzz_corpus/json_body/` | `fuzz_json_body` |
| `backend/fuzz_corpus/cart_payload/` | `fuzz_cart_payload` |
| `backend/fuzz_corpus/search_term/` | `fuzz_search_term` |
| `backend/fuzz_corpus/cookie_parser/` | `fuzz_cookie_parser` |

From `backend/build/`:

```bash
./fuzz_url_decode ../fuzz_corpus/url_decode
./fuzz_json_body ../fuzz_corpus/json_body
./fuzz_cart_payload ../fuzz_corpus/cart_payload
./fuzz_search_term ../fuzz_corpus/search_term
./fuzz_cookie_parser ../fuzz_corpus/cookie_parser
./fuzz_json_parser ../fuzz_corpus/json
./fuzz_query_builder ../fuzz_corpus/query_builder
./fuzz_product_search ../fuzz_corpus/product_search   # crash in ~10s (lab bug)
```

## How to run the fuzzers

Run from the `build` directory (or ensure any paths you pass are correct).

### Quick run (no corpus)

```bash
./fuzz_url_decode
./fuzz_json_body
./fuzz_cart_payload
./fuzz_search_term
./fuzz_cookie_parser
./fuzz_json_parser
./fuzz_query_builder
./fuzz_product_search
```

Each will run until you stop it (Ctrl+C). Use the **seed corpus** above for faster coverage and a reproducible crash demo.

### Run with the provided corpus (recommended)

```bash
# From backend/build/
./fuzz_json_parser ../fuzz_corpus/json
./fuzz_query_builder ../fuzz_corpus/query_builder
./fuzz_product_search ../fuzz_corpus/product_search
```

`fuzz_product_search` run with `../fuzz_corpus/product_search` should report a crash within about 10 seconds (intentional buffer overflow triggered by the `crash_trigger` seed).

### Run with a timeout or iteration limit

```bash
./fuzz_json_parser -max_total_time=60 corpus_json   # run for 60 seconds
./fuzz_json_parser -runs=10000 corpus_json          # run 10000 inputs then exit
```

### Reproduce a crash from a file

If the fuzzer finds a crash, it writes the crashing input to a file (see below). To reproduce:

```bash
./fuzz_json_parser /path/to/crash-file
./fuzz_json_parser -runs=1 /path/to/crash-file
```

The same binary can be used to run a single input; AddressSanitizer will report the same failure.

## Where crashes are shown

- **Terminal**: Crashes (e.g. ASan reports, assertion failures) are printed to **stderr** in the same terminal where you started the fuzzer.
- **Crash artifacts**: libFuzzer writes the **minimal crashing input** to a file in the current working directory, e.g.:
  - `crash-<hash>`
  - `leak-<hash>` (if using LeakSanitizer)
  - `timeout-<hash>` (if a timeout is detected)

To get a **readable stack trace**, build with `-g` and `-fno-omit-frame-pointer` (the provided CMake adds the latter).

## Expected crash output examples

### 1. fuzz_product_search (lab crash trigger)

Running `./fuzz_product_search ../fuzz_corpus/product_search` triggers an intentional buffer overflow. You should see output similar to:

```
INFO: Running with entropic power schedule (0xFF, 100).
INFO: Seed: ...
...
==XXXXX==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffc12345678 at pc 0x... bp 0x... sp 0x...
WRITE of size 20 at 0x7ffc12345678 thread T0
    #0 0x... in memcpy
    #1 0x... in lab_crash_if_triggered fuzz_targets/fuzz_product_search.cpp:...
    #2 0x... in LLVMFuzzerTestOneInput fuzz_targets/fuzz_product_search.cpp:...
    ...
SUMMARY: AddressSanitizer: stack-buffer-overflow ...
```

The crash file is written as `crash-<hash>` in the current directory; its content starts with `CRASHME`.

### 2. ASan report (generic)

Any AddressSanitizer finding will look roughly like:

```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x60300000eff4 at pc 0x55... bp 0x7ff... sp 0x7ff...
READ of size 1 at 0x60300000eff4 thread T0
    #0 0x55... in SomeFunction path/to/source.cpp:42:5
    #1 0x55... in LLVMFuzzerTestOneInput path/to/fuzz_xxx.cpp:20:5
    ...
Shadow bytes around the buggy address:
  0x0c067fff9da0: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  ...
==12345==ABORTING
MS: 0 ; base unit: ...
artifact_prefix='./'; Test unit written to ./crash-...
```

### 3. Reproducing a crash

After a crash, run the same binary on the artifact to get the same report:

```bash
./fuzz_product_search ./crash-abc123def456
```

## Running without libFuzzer (custom fuzz loop)

Each fuzz target includes a **custom main** when not built with the fuzzer (i.e. when `FUZZING_BUILD_MODE` is not defined). That main reads one file from disk and calls the same logic as the fuzzer, so you can reproduce crashes or run a single input without libFuzzer. To build that way, compile the `.cpp` file **without** `-DFUZZING_BUILD_MODE` and **without** `-fsanitize=fuzzer`, then link (and for `fuzz_json_parser`, link with Crow). Run: `./fuzz_json_parser path/to/input.txt`. For normal fuzzing, use the CMake fuzz targets with Clang and `BUILD_FUZZ_TARGETS=ON` as above.
