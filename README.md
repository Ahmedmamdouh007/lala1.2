# Lala Store - Clothing E-Commerce

Full-stack clothing e-commerce website with C++ backend (Crow framework), React frontend (Vite), and PostgreSQL database.

## Project Structure

```
backend/          # C++ Crow API
frontend/         # React Vite app
database/         # SQL schema & seed
docker-compose.yml
```

---

## CTF-style checklist

Use this checklist to practice security tooling and lab targets (all local / educational).

| # | Task | Goal |
|---|------|------|
| **1** | **Trigger an ASan crash using lab_targets** | Build and run `backend/lab_targets/asan_demo.cpp` with AddressSanitizer; observe the deterministic heap-buffer-overflow report. See [ASan Demo (Educational)](#asan-demo-educational) for build/run. |
| **2** | **Fuzz the JSON parser until it crashes** | Build fuzz targets with `BUILD_FUZZ_TARGETS=ON`, run `fuzz_json_parser` (with or without `backend/fuzz_corpus/json`). Let it run until it finds a crash or you hit a time limit. See [Fuzzing (backend)](#fuzzing-backend) and [backend/fuzz_targets/README.md](backend/fuzz_targets/README.md). |
| **3** | **Send payloads to `/lab/sqli/*` and observe simulated results** | With `ENABLE_LABS=ON` and `LAB_MODE=true`, send requests to `GET /lab/sqli/search?term=...` and `GET /lab/sqli/product?id=...`. Try normal inputs, then time-based style payloads (e.g. `term=sleep`, `id=1; SELECT pg_sleep(1)--`) and compare `response_time_ms` and `simulated_time_based_sqli` in the JSON. See [Lab mode (training endpoints)](#lab-mode-training-endpoints). |
| **4** | **Use boofuzz to break the tcp_lab_server parser (ASan crash)** | Build `backend/lab_targets/tcp_lab_server` with ASan, run it, then run the boofuzz script against it until AddressSanitizer reports a crash. See [TCP lab server (boofuzz)](#tcp-lab-server-boofuzz) below. |

---

## Prerequisites

- Docker & Docker Compose
- CMake 3.14+, Make
- C++17 compiler (g++, clang++, or MSVC)
- Node.js 18+
- PostgreSQL dev libraries & libpqxx
- OpenSSL
- pkg-config (Linux)

## One-command setup

Run the full setup (Docker, backend-node, frontend, optional C++ build, API tests):

- **Windows:** `npm run setup:win` or `.\setup.ps1`
- **macOS/Linux:** `chmod +x setup.sh && npm run setup:mac` or `./setup.sh`

This installs dependencies, builds the frontend, optionally builds the C++ backend with labs, and runs API smoke tests.

## Quick Start

### 1. Start PostgreSQL

```bash
docker compose up -d
```

Wait 10–15 seconds for the database to initialize. Schema and seed run automatically via `docker-entrypoint-initdb.d`.

### 2. Run Backend (Node.js – recommended)

```bash
cd backend-node
npm install
npm start
```

Backend runs at `http://localhost:8080` and connects to PostgreSQL (port 5434).

### 3. Run Frontend

```bash
cd frontend
npm install
npm run dev
```

Frontend runs at `http://localhost:3006` and proxies `/api` to the backend.

### 4. Alternative: C++ Backend

To use the C++ (Crow) backend instead, install libpqxx and OpenSSL (e.g. via vcpkg), then:

```bash
cd backend
mkdir build && cd build
cmake ..
cmake --build .
cd ..
./build/lala_backend    # or lala_backend.exe on Windows
```

## Database

- **Database:** lala_store
- **Port:** 5434 (host) → 5432 (container)

Two roles are created by `database/roles.sql` on init:
- **app_user** / **app_pass** – used by the C++ backend for normal routes (full access).
- **lab_readonly** / **lab_readonly_pass** – used by the C++ backend for `/lab` routes (SELECT only on `products` and `categories`, no `users` access).

Configure in `backend/config/db_config.json`: `user`/`password` for app, `lab_user`/`lab_password` for lab. If the DB was created before `roles.sql` existed, create the roles manually: `docker exec -i lala_store_db psql -U postgres -d lala_store < database/roles.sql`.

### Tables

- `users` – auth
- `categories` – Men, Women
- `products` – 24 clothing items with images, prices, descriptions
- `cart_items` – user cart
- `orders`, `order_items` – orders

## API Endpoints

### Products

- `GET /api/products` – all products
- `GET /api/products/:id` – product by ID
- `GET /api/products/category/:categoryName` – by category (Men, Women)
- `GET /api/products/search?q=` – search by name/description

### Auth

- `POST /api/auth/register` – `{ "email", "password", "name" }`
- `POST /api/auth/login` – `{ "email", "password" }`

### Cart

- `GET /api/cart/:userId` – cart items
- `POST /api/cart/add` – `{ "user_id", "product_id", "quantity" }`
- `POST /api/cart/remove` – `{ "user_id", "product_id" }`
- `POST /api/cart/update_quantity` – `{ "user_id", "product_id", "quantity" }`

### Orders

- `POST /api/orders/create` – `{ "user_id", "items": [{ "product_id", "quantity" }] }`
- `GET /api/orders/:userId` – user orders

## Lab mode (training endpoints)

### Compile-time flag: `ENABLE_LABS`

The security lab module is **optional**. Enable it at build time:

```bash
cd backend
mkdir -p build && cd build
cmake -DENABLE_LABS=ON ..
cmake --build .
```

Without `-DENABLE_LABS=ON`, the lab routes are not compiled and the binary has no lab endpoints. Lab code lives under **`backend/lab/`** (sql, memory, fuzz) and **`backend/routes/lab_routes.cpp`**.

### Runtime env: `LAB_MODE`

When the backend is built with `ENABLE_LABS=ON`, set **`LAB_MODE=true`** to enable lab endpoints:

- **`LAB_MODE=false`** (default): Lab routes are registered but the **middleware guard** returns **404** for all lab requests (so they appear off).
- **`LAB_MODE=true`**: Lab routes respond. A **huge banner** is printed on server start. The guard allows only **localhost** (127.0.0.1 or ::1); requests from any other IP get **403 Forbidden**.

### How to enable LAB_MODE

```bash
# Linux/macOS
export LAB_MODE=true
./build/lala_backend

# Windows PowerShell
$env:LAB_MODE="true"
./build/lala_backend.exe
```

### How to test lab endpoints locally only

1. Build with lab support: `cmake -DENABLE_LABS=ON ..` then build.
2. Start the backend with `LAB_MODE=true` (see above); a large banner will appear.
3. Call the lab endpoints **from the same machine** (localhost):

   ```bash
   # Search (training example: unsafe query building)
   curl "http://127.0.0.1:8080/lab/sqli/search?term=shirt"

   # Product by id (same training example)
   curl "http://127.0.0.1:8080/lab/sqli/product?id=1"
   ```

4. Responses include `"lab_mode": true` and `"warning": "Do not deploy this"`.
5. From another machine (e.g. another host or IP), the same URLs return **403** with a message that lab endpoints are only available from localhost.

**Do not deploy** with `LAB_MODE=true` on a publicly reachable server; lab endpoints are intended for local training only.

### Lab endpoints (when `ENABLE_LABS=ON` and `LAB_MODE=true`)

| Endpoint | Description |
|----------|-------------|
| `GET /lab/sqli/search?term=` | Unsafe query building (concatenation). Read-only DB, query timeout, max one query. |
| `GET /lab/sqli/product?id=` | Same; product by id. |
| `GET /lab/sqli/error_based?term=` | **Error-based SQLi training:** simulates DB error leakage (fake error or real syntax error from concatenation). |
| `GET /lab/sqli/boolean_based?term=` | **Boolean-based SQLi training:** different result sizes for true-like (`1=1`, `'1'='1'`) vs false-like (`1=2`) payloads. |
| `GET /lab/sqli/time_based?term=` | **Time-based SQLi training:** simulates 1s delay when payload contains `sleep`/`pg_sleep`/`benchmark`/`waitfor` (no DB sleep). |
| `GET /lab/sqli/union_based?term=` | **Union-based SQLi training:** when payload looks like `UNION SELECT`, response includes a simulated “leaked” row. |
| `GET /lab/sqli/auth_bypass?email=&password=` | **Auth-bypass SQLi training:** simulates login success when email/password contain classic bypass payloads (e.g. `' OR '1'='1`); no real users table. |
| `GET /lab/sqli/order_by?column=` | **Order-by SQLi training:** `column` concatenated into `ORDER BY` (unsafe). |
| `GET /lab/sqli/limit?n=` | **Limit SQLi training:** `n` concatenated into `LIMIT` (unsafe). |

All return JSON with `lab_mode`, `warning`, and `training_lab`; simulation endpoints include `sqli_type` and `lab_message`. For extra safety, use a **database role with read-only permissions** (e.g. a PostgreSQL user that can only `SELECT` on `products` and `categories`) in your lab config when testing these endpoints.

**Detection and teaching behaviour:**

- **Time-based SQLi simulation:** If the request contains a payload that looks like a sleep (e.g. `sleep`, `pg_sleep`, `benchmark`, `waitfor`), the server **simulates** a 1-second delay and returns `response_time_ms: 1000` and `simulated_time_based_sqli: true` **without executing any dangerous DB functions**. Normal requests return `response_time_ms: 0`. This makes time-based detection obvious for teaching.
- **Clear error messages in lab mode:** On errors (e.g. invalid SQL, missing parameter), responses include a `lab_message` field with a short explanation for teaching (e.g. "Use parameterized queries to avoid injection").

### Educational endpoints (no DB)

When `ENABLE_LABS=ON` and `LAB_MODE=true`, these read-only educational endpoints are available (same guard: localhost only):

- `GET /api/lab/sql_injection_explained` – JSON explanation of SQL injection
- `GET /api/lab/memory_safety_explained` – JSON explanation of buffer overflow, ASan, and safe alternatives

### Lab telemetry (logging)

When lab mode is on, every request to a `/lab` or `/api/lab` endpoint is logged to a **lab-only** log file. No passwords or sensitive parameter values are stored.

**Log file location**

- Default path: **`logs/lab.log`** relative to the process current working directory.
- If you run the backend from the **backend** directory: `backend/logs/lab.log` (the `logs` directory is created automatically on first write).
- If you run from the project root: `./logs/lab.log`.

**How to view logs**

```bash
# Follow new entries (Linux/macOS)
tail -f logs/lab.log

# Or open the file (Windows: type logs\lab.log, or open in an editor)
cat logs/lab.log
```

Each line has: **timestamp**, **endpoint**, **params** (query string with password-like params redacted as `[REDACTED]`), **injection** (pattern name or `none`), **response** (HTTP status code).

**What the patterns mean**

| `injection` value   | Meaning |
|---------------------|--------|
| `none`              | No injection-style payload was detected; normal handling or guard (403/404). |
| `time_based`        | Input looked like a time-based SQLi payload (e.g. `sleep`, `pg_sleep`, `benchmark`, `waitfor`); the lab simulated a delay. |
| `error_based`       | Input looked like an error-based SQLi payload (quotes, `extractvalue`, `updatexml`, etc.); the lab returned a simulated or real error. |
| `boolean_true`      | Input looked like a boolean true condition (e.g. `1=1`, `'1'='1'`); the lab returned the full result set. |
| `boolean_false`     | Input looked like a boolean false condition (e.g. `1=2`); the lab returned an empty set. |
| `union_based`       | Input looked like a UNION-based payload; the lab added a simulated “leaked” row. |
| `auth_bypass`       | Email or password contained an auth-bypass style payload (e.g. `' OR '1'='1`); the lab simulated a successful login. |

**Response codes** in the log are the HTTP status sent to the client (e.g. `200`, `400`, `401`, `403`, `404`, `500`). Logs are append-only and do **not** store password values.

## ASan Demo (Educational)

The file `backend/lab_targets/asan_demo.cpp` is not part of the web server. It triggers **one deterministic** buffer overflow so the same crash is reported every run.

### Build with ASan and symbols (readable stack traces)

```bash
# From project root. Use -g and -fno-omit-frame-pointer for clear stack traces.
clang++ -fsanitize=address -g -fno-omit-frame-pointer -o asan_demo backend/lab_targets/asan_demo.cpp

# Or with g++
g++ -fsanitize=address -g -fno-omit-frame-pointer -o asan_demo backend/lab_targets/asan_demo.cpp
```

### Run

```bash
./asan_demo
```

The crash is deterministic (same every run). Expected output (example):

```
ASan Demo - deterministic heap buffer overflow (educational)
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x... at pc 0x... bp 0x... sp 0x...
WRITE of size 12 at 0x... thread T0
    #0 0x... in heap_buffer_overflow_demo() asan_demo.cpp:...
    #1 0x... in main asan_demo.cpp:...
SUMMARY: AddressSanitizer: heap-buffer-overflow ...
==12345==ABORTING
```

## TCP lab server (boofuzz)

A minimal TCP server with a **vulnerable parser** is provided for Task 4: fuzz it with [boofuzz](https://github.com/jtpereyda/boofuzz) until AddressSanitizer reports a crash.

- **Server:** `backend/lab_targets/tcp_lab_server.cpp` — listens on port **9999**, reads a line into a small buffer (intentional overflow when the line is long). **Build with ASan** so crashes are detected.
- **Fuzzer:** `backend/lab_targets/boofuzz_tcp_lab.py` — boofuzz script that sends fuzz payloads to `localhost:9999`.

### Build and run the TCP lab server (with ASan)

```bash
# From project root. Use -g for readable stack traces.
clang++ -fsanitize=address -g -fno-omit-frame-pointer -o tcp_lab_server backend/lab_targets/tcp_lab_server.cpp

# Run (listens on 0.0.0.0:9999)
./tcp_lab_server
```

### Run boofuzz against it

In another terminal:

```bash
pip install boofuzz
python backend/lab_targets/boofuzz_tcp_lab.py
```

Keep the server running under ASan; when boofuzz sends a long enough payload, the server’s parser overflows and ASan will report the crash in the server terminal. That completes Task 4.

## Fuzzing (backend)

Standalone libFuzzer targets live in **`backend/fuzz_targets/`**. They fuzz JSON parsing, input validation, and product search term processing (no web server). A **seed corpus** in **`backend/fuzz_corpus/`** is provided; running `fuzz_product_search` with that corpus should trigger a crash within about 10 seconds (intentional lab bug for teaching).

See **[backend/fuzz_targets/README.md](backend/fuzz_targets/README.md)** for:

- Build: `cmake -DBUILD_FUZZ_TARGETS=ON -DCMAKE_CXX_COMPILER=clang++ ..` in `backend/build`
- How to run each fuzzer with `backend/fuzz_corpus/<target>`
- **Expected crash output examples** (ASan stack traces, crash artifacts)

## Testing Endpoints

```bash
# Get all products
curl http://localhost:8080/api/products

# Get product by ID
curl http://localhost:8080/api/products/1

# Register
curl -X POST http://localhost:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@test.com","password":"test123","name":"Test User"}'

# Login
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"test@test.com","password":"test123"}'
```

## Frontend Pages

- **Home** – featured products
- **Men** – men's collection
- **Women** – women's collection
- **Product Details** – single product, add to cart
- **Cart** – cart items (requires login)
- **Checkout** – place order
- **Login / Register** – auth

## Troubleshooting

### Backend won't build

- Ensure libpqxx and OpenSSL are installed:
  - **Ubuntu:** `sudo apt install libpqxx-dev libssl-dev pkg-config`
  - **macOS:** `brew install libpqxx openssl pkg-config`
  - **Windows:** Use vcpkg: `vcpkg install libpqxx openssl`

### Database connection failed

- Confirm Docker container is running: `docker ps`
- Wait 10–15 seconds after `docker compose up -d` before starting the backend
- Check `backend/config/db_config.json` matches Docker (host: localhost, port: 5432)

### CORS

The backend serves the API. The Vite dev server proxies `/api` to the backend, so CORS is not an issue during development.
