# Security lab module

This directory holds the **security lab** components. Lab routes are only compiled when `ENABLE_LABS=ON` and only respond when `LAB_MODE=true`; they accept requests from **127.0.0.1** only.

- **sql/** – SQL / injection lab helpers and logic.
- **memory/** – Memory-safety lab helpers.
- **fuzz/** – Fuzzing-related lab helpers.
- **telemetry/** – Lab-only request logging (endpoint, redacted params, injection pattern, response code) to `logs/lab.log`. Passwords are never stored.

HTTP routes that expose these labs live in **`backend/routes/lab_routes.cpp`**.

For how to view logs and what the logged **injection** patterns mean, see the main [Lab telemetry (logging)](../../README.md#lab-telemetry-logging) section in the project README.
