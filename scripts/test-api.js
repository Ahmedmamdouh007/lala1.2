#!/usr/bin/env node
/**
 * API smoke test - verifies backend endpoints respond correctly.
 * Run after backend is started. Works on Windows and Mac.
 * Usage: node scripts/test-api.js [baseUrl]
 */
const baseUrl = process.argv[2] || 'http://127.0.0.1:8080';

async function fetchJson(url, options = {}) {
  const res = await fetch(url, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...options.headers },
  });
  const text = await res.text();
  let json;
  try {
    json = text ? JSON.parse(text) : null;
  } catch {
    throw new Error(`Invalid JSON from ${url}: ${text.slice(0, 200)}`);
  }
  return { status: res.status, json };
}

async function runTests() {
  let passed = 0;
  let failed = 0;

  const ok = (name) => {
    console.log(`  ✓ ${name}`);
    passed++;
  };
  const fail = (name, err) => {
    console.log(`  ✗ ${name}: ${err.message || err}`);
    failed++;
  };

  console.log(`\nTesting API at ${baseUrl}\n`);

  // GET /api/products
  try {
    const { status, json } = await fetchJson(`${baseUrl}/api/products`);
    if (status !== 200 || !json?.success || !Array.isArray(json?.data)) {
      throw new Error(`Expected { success: true, data: [...] }, got status ${status}`);
    }
    ok('GET /api/products');
  } catch (e) {
    fail('GET /api/products', e);
  }

  // GET /api/products/:id
  try {
    const { status, json } = await fetchJson(`${baseUrl}/api/products/1`);
    if (status !== 200 || !json?.success || !json?.data?.id) {
      throw new Error(`Expected product object, got status ${status}`);
    }
    ok('GET /api/products/1');
  } catch (e) {
    fail('GET /api/products/1', e);
  }

  // POST /api/auth/register + login (register fresh user, then login)
  const testEmail = `test-setup-${Date.now()}@example.com`;
  const testPassword = 'testpass123';
  try {
    const { status, json } = await fetchJson(`${baseUrl}/api/auth/register`, {
      method: 'POST',
      body: JSON.stringify({
        email: testEmail,
        password: testPassword,
        name: 'Test User',
      }),
    });
    if (status !== 200 || !json?.success) {
      throw new Error(`Expected success, got status ${status}: ${JSON.stringify(json)}`);
    }
    ok('POST /api/auth/register');
  } catch (e) {
    fail('POST /api/auth/register', e);
  }

  try {
    const { status, json } = await fetchJson(`${baseUrl}/api/auth/login`, {
      method: 'POST',
      body: JSON.stringify({ email: testEmail, password: testPassword }),
    });
    if (status !== 200 || !json?.success) {
      throw new Error(`Expected success, got status ${status}`);
    }
    ok('POST /api/auth/login');
  } catch (e) {
    fail('POST /api/auth/login', e);
  }

  console.log(`\nAPI tests: ${passed} passed, ${failed} failed\n`);
  return failed === 0;
}

runTests()
  .then((success) => process.exit(success ? 0 : 1))
  .catch((err) => {
    console.error('Test runner error:', err);
    process.exit(1);
  });
