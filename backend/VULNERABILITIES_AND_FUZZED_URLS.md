# Vulnerabilities and Fuzzed URLs Reference

Sorted by **category** (Buffer overflow / memory, then SQL injection), then **file path**, then **line number**.  
Each entry includes: vulnerability type, file location (path:line), and related URL or fuzz target link.

---

## Summary – full list of vulnerabilities

### Buffer overflow & memory (15)

| # | Vulnerability | Location |
|---|----------------|----------|
| 1 | Stack buffer overflow (strcpy) | backend/lab_targets/stack_buffer_overflow.cpp:13 |
| 2 | Heap buffer overflow (strcpy) | backend/lab_targets/heap_buffer_overflow.cpp:13 |
| 3 | Heap buffer overflow (ASan demo) | backend/lab_targets/asan_demo.cpp:20 |
| 4 | Stack buffer overflow (recv) | backend/lab_targets/tcp_lab_server.cpp:35 |
| 5 | Stack buffer overflow (buf[n] = '\\0') | backend/lab_targets/tcp_lab_server.cpp:37 |
| 6 | Format string / stack overflow (sprintf) | backend/lab_targets/format_string_bug.cpp:15 |
| 7 | Integer overflow → heap OOB | backend/lab_targets/integer_overflow_to_alloc.cpp:14–17 |
| 8 | Out-of-bounds read (heap) | backend/lab_targets/out_of_bounds_read.cpp:13 |
| 9 | Heap buffer overflow (fuzz CRASHME) | backend/fuzz_targets/fuzz_product_search.cpp:44–46 |
| 10 | Unsafe strncpy / read past buffer | backend/lab_targets/unsafe_strncpy.cpp:18–20 |
| 11 | Stack read overflow (OOB read) | backend/lab_targets/stack_read_overflow.cpp:12 |
| 12 | Stack buffer overflow (memcpy) | backend/lab_targets/fixed_buffer_overflow.cpp:15 |
| 13 | Use-after-free | backend/lab_targets/use_after_free.cpp:13 |
| 14 | Double free | backend/lab_targets/double_free.cpp:13 |
| 15 | Null pointer dereference | backend/lab_targets/null_deref.cpp:11 |

### SQL injection (9)

| # | Endpoint | Location |
|---|----------|----------|
| 1 | /lab/sqli/search | backend/routes/lab_routes.cpp:166–170 |
| 2 | /lab/sqli/product | backend/routes/lab_routes.cpp:225–230 |
| 3 | /lab/sqli/error_based | backend/routes/lab_routes.cpp:276–281 |
| 4 | /lab/sqli/boolean_based | backend/routes/lab_routes.cpp:317–322 |
| 5 | /lab/sqli/time_based | backend/routes/lab_routes.cpp:392–397 |
| 6 | /lab/sqli/union_based | backend/routes/lab_routes.cpp:431–436 |
| 7 | /lab/sqli/auth_bypass (simulated) | backend/routes/lab_routes.cpp:466–496 |
| 8 | /lab/sqli/order_by | backend/routes/lab_routes.cpp:517 |
| 9 | /lab/sqli/limit | backend/routes/lab_routes.cpp:554 |

---

## 1. Buffer overflow (and related memory) vulnerabilities

| # | Type              | File | Line | Description | URL / Link |
|---|-------------------|------|------|-------------|------------|
| 1 | Stack buffer overflow | backend/lab_targets/stack_buffer_overflow.cpp | 13 | `strcpy(buf, "overflow")` into 4-byte buffer | N/A (standalone binary; run with ASan) |
| 2 | Heap buffer overflow  | backend/lab_targets/heap_buffer_overflow.cpp  | 13 | `strcpy(p, "Hello World")` into 4-byte heap buffer | N/A (standalone binary; run with ASan) |
| 3 | Heap buffer overflow  | backend/lab_targets/asan_demo.cpp            | 20 | `strcpy(buf, "Hello World")` into 4-byte buffer (deterministic ASan demo) | N/A (standalone binary) |
| 4 | Stack buffer overflow | backend/lab_targets/tcp_lab_server.cpp       | 35 | `recv(fd, buf, 4096, 0)` into 16-byte buffer | **TCP (fuzzed):** `tcp://127.0.0.1:9999` (boofuzz: backend/lab_targets/boofuzz_tcp_lab.py) |
| 5 | Stack buffer overflow | backend/lab_targets/tcp_lab_server.cpp       | 37 | `buf[n] = '\0'` when n >= PARSER_BUF_SIZE | Same as above |
| 6 | Format string / stack overflow | backend/lab_targets/format_string_bug.cpp | 15 | `sprintf(buf, "%s", "overflow")` into 4-byte buffer | N/A (standalone binary) |
| 7 | Integer overflow → heap OOB | backend/lab_targets/integer_overflow_to_alloc.cpp | 14–17 | Undersized alloc then `arr[100] = 2` | N/A (standalone binary) |
| 8 | Out-of-bounds read   | backend/lab_targets/out_of_bounds_read.cpp    | 13 | `arr[10]` on 5-element array | N/A (standalone binary) |
| 9 | Heap buffer overflow (fuzz-triggered) | backend/fuzz_targets/fuzz_product_search.cpp | 44–46 | `lab_crash_if_triggered`: memcpy into 4-byte buffer when input starts with "CRASHME" | **Fuzzed (no HTTP):** search-term logic used by `GET /lab/sqli/search?term=` and product search; run: `./fuzz_product_search ../fuzz_corpus/product_search` |
| 10 | Unsafe strncpy / read past buffer | backend/lab_targets/unsafe_strncpy.cpp | 18–20 | `strncpy(buf, long_src, sizeof(buf))` no null terminator; then use as string | N/A (standalone binary; run with ASan) |
| 11 | Stack read overflow | backend/lab_targets/stack_read_overflow.cpp | 12 | `stack_arr[10]` on 4-element stack array (OOB read) | N/A (standalone binary; run with ASan) |
| 12 | Stack buffer overflow | backend/lab_targets/fixed_buffer_overflow.cpp | 15 | `memcpy(buf, src, strlen(src)+1)` into 6-byte buffer (12 bytes copied) | N/A (standalone binary; run with ASan) |
| 13 | Use-after-free | backend/lab_targets/use_after_free.cpp | 13 | `*p = 99` after `delete p` | N/A (standalone binary; run with ASan) |
| 14 | Double free | backend/lab_targets/double_free.cpp | 13 | `delete p` twice | N/A (standalone binary; run with ASan) |
| 15 | Null pointer dereference | backend/lab_targets/null_deref.cpp | 11 | `*p = 42` with `p = nullptr` | N/A (standalone binary; run with ASan) |

---

## 2. SQL injection vulnerabilities (unsafe concatenation)

All in **backend/routes/lab_routes.cpp**. User input is concatenated into SQL; in production use `exec_params` with placeholders.

| # | Endpoint | File | Line(s) | Concatenated param | URL |
|---|----------|------|--------|--------------------|-----|
| 1 | /lab/sqli/search | backend/routes/lab_routes.cpp | 166–170 | `term` | http://127.0.0.1:8080/lab/sqli/search?term= |
| 2 | /lab/sqli/product | backend/routes/lab_routes.cpp | 225–230 | `idStr` (id) | http://127.0.0.1:8080/lab/sqli/product?id= |
| 3 | /lab/sqli/error_based | backend/routes/lab_routes.cpp | 276–281 | `term` | http://127.0.0.1:8080/lab/sqli/error_based?term= |
| 4 | /lab/sqli/boolean_based | backend/routes/lab_routes.cpp | 317–322 | `term` | http://127.0.0.1:8080/lab/sqli/boolean_based?term= |
| 5 | /lab/sqli/time_based | backend/routes/lab_routes.cpp | 392–397 | `term` | http://127.0.0.1:8080/lab/sqli/time_based?term= |
| 6 | /lab/sqli/union_based | backend/routes/lab_routes.cpp | 431–436 | `term` | http://127.0.0.1:8080/lab/sqli/union_based?term= |
| 7 | /lab/sqli/auth_bypass | backend/routes/lab_routes.cpp | 466–496 | email, password (simulated only; no SQL built from params) | http://127.0.0.1:8080/lab/sqli/auth_bypass?email=&password= |
| 8 | /lab/sqli/order_by | backend/routes/lab_routes.cpp | 517 | `column` (ORDER BY concatenation) | http://127.0.0.1:8080/lab/sqli/order_by?column= |
| 9 | /lab/sqli/limit | backend/routes/lab_routes.cpp | 554 | `nStr` (LIMIT concatenation) | http://127.0.0.1:8080/lab/sqli/limit?n= |

*Note:* Endpoints 1–6 and 8–9 **concatenate** user input into `txn.exec(sql)` (vulnerable pattern). `/lab/sqli/auth_bypass` only **simulates** auth-bypass (no real SQL built from email/password); it is listed as the training URL for injection-style payloads in login params.

---

## 3. Fuzz targets and related URLs

These fuzz **internal parsing** (no live HTTP). The table maps each fuzz target to the API/lab URLs whose input flows they exercise.

| Fuzz target | File | Fuzzed logic | Related URL(s) |
|------------|------|--------------|----------------|
| fuzz_json_parser | backend/fuzz_targets/fuzz_json_parser.cpp | crow::json::load() | Any POST with JSON body (e.g. /api/auth/login, /api/cart/add, /api/orders/create) |
| fuzz_query_builder | backend/fuzz_targets/fuzz_query_builder.cpp | ID sanitization, search term, LIKE pattern | /api/products/search?q= , /lab/sqli/search?term= , /lab/sqli/product?id= |
| fuzz_product_search | backend/fuzz_targets/fuzz_product_search.cpp | Search term processing + **buffer overflow** (CRASHME) | http://127.0.0.1:8080/lab/sqli/search?term= , /api/products/search?q= |
| fuzz_search_term | backend/fuzz_targets/fuzz_search_term.cpp | Search term length/trim/pattern | http://127.0.0.1:8080/lab/sqli/search?term= , /api/products/search?q= |
| fuzz_url_decode | backend/fuzz_targets/fuzz_url_decode.cpp | URL percent-decode | Any URL with encoded params (e.g. ?term=%41%42) |
| fuzz_json_body | backend/fuzz_targets/fuzz_json_body.cpp | JSON body (user_id, items, etc.) | /api/orders/create , /api/cart/add , etc. |
| fuzz_cart_payload | backend/fuzz_targets/fuzz_cart_payload.cpp | Cart JSON (user_id, product_id, quantity) | /api/cart/add , /api/cart/update_quantity |
| fuzz_cookie_parser | backend/fuzz_targets/fuzz_cookie_parser.cpp | Cookie header parsing | Any request with Cookie header |
| (TCP, boofuzz) | backend/lab_targets/boofuzz_tcp_lab.py | TCP lab server parser | tcp://127.0.0.1:9999 (TCP, not HTTP) |

---

## 4. Quick link list (lab SQLi + fuzzed TCP)

**Lab SQLi endpoints (localhost only; require ENABLE_LABS=ON and LAB_MODE=true):**

- http://127.0.0.1:8080/lab/sqli/search?term=
- http://127.0.0.1:8080/lab/sqli/product?id=
- http://127.0.0.1:8080/lab/sqli/error_based?term=
- http://127.0.0.1:8080/lab/sqli/boolean_based?term=
- http://127.0.0.1:8080/lab/sqli/time_based?term=
- http://127.0.0.1:8080/lab/sqli/union_based?term=
- http://127.0.0.1:8080/lab/sqli/auth_bypass?email=&password=
- http://127.0.0.1:8080/lab/sqli/order_by?column=
- http://127.0.0.1:8080/lab/sqli/limit?n=

**Buffer overflow – fuzzed TCP (run tcp_lab_server on 9999, then boofuzz):**

- tcp://127.0.0.1:9999 (script: backend/lab_targets/boofuzz_tcp_lab.py)

---

*Generated from codebase scan. Lab endpoints are for training only; do not deploy with LAB_MODE=true.*
