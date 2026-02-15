# Validation demo module

Utility module that demonstrates **bad** vs **correct** input validation for lab training.

## Functions (see `validation_demo.h`)

**Bad examples (what NOT to do):**
- `bad_no_length_check()` – accepts any length (buffer overflow / DoS risk).
- `bad_concatenate_into_sql(term)` – builds SQL by concatenation (injection risk).
- `bad_weak_quote_check()` – only checks for one quote (easy to bypass).

**Correct examples (what TO do):**
- `correct_length_check(s, max_len)` – enforce max length (default 500).
- `correct_parameterized_usage()` – use bound parameters, never concatenate.
- `correct_has_control_chars(s)` – detect control characters to reject or strip.

## Endpoint

**`GET /lab/validation_demo?input=...`** (when `ENABLE_LABS=ON` and `LAB_MODE=true`, localhost only)

Response JSON:
- **`input_length`** – length of the `input` parameter.
- **`is_dangerous`** – whether the input is considered dangerous (length, control chars, SQL-like, script-like).
- **`reason`** – short explanation (why dangerous or "Safe for demo").
- **`how_to_fix`** – recommendation (e.g. enforce max length, use parameterized queries, escape output).

Example:
```bash
curl "http://127.0.0.1:8080/lab/validation_demo?input=hello"
curl "http://127.0.0.1:8080/lab/validation_demo?input=' OR 1=1--"
```
