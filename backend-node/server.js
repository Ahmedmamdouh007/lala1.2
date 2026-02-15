const express = require('express');
const cors = require('cors');
const { Pool } = require('pg');
const crypto = require('crypto');
const path = require('path');
const fs = require('fs');

const app = express();
app.use(cors());
app.use(express.json());

// Load DB config
let dbConfig = {
  host: process.env.PG_HOST || 'localhost',
  port: parseInt(process.env.PG_PORT || '5434', 10),
  database: process.env.PG_DATABASE || 'lala_store',
  user: process.env.PG_USER || 'postgres',
  password: process.env.PG_PASSWORD || 'postgres',
};

const configPath = path.join(__dirname, '..', 'backend', 'config', 'db_config.json');
// Also try backend-node/config if backend doesn't exist
const altConfigPath = path.join(__dirname, 'config', 'db_config.json');
const cfgFile = fs.existsSync(configPath) ? configPath : (fs.existsSync(altConfigPath) ? altConfigPath : null);
if (cfgFile) {
  try {
    const json = JSON.parse(fs.readFileSync(cfgFile, 'utf8'));
    dbConfig = {
      host: json.host || dbConfig.host,
      port: json.port || dbConfig.port,
      database: json.dbname || dbConfig.database,
      user: json.user || dbConfig.user,
      password: json.password || dbConfig.password,
    };
  } catch (e) {
    console.warn('Could not load db_config.json, using defaults');
  }
}

const pool = new Pool(dbConfig);

pool.on('error', (err) => {
  console.error('Unexpected DB error:', err);
});

function sha256(str) {
  return crypto.createHash('sha256').update(str).digest('hex');
}

function success(data) {
  return { success: true, data };
}

function error(msg) {
  return { success: false, error: msg };
}

// Products
app.get('/api/products', async (req, res) => {
  try {
    const r = await pool.query(
      `SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock,
       c.name as category_name, p.created_at
       FROM products p LEFT JOIN categories c ON p.category_id = c.id ORDER BY p.id`
    );
    res.json(success(r.rows));
  } catch (e) {
    console.error('GET /api/products:', e);
    res.status(500).json(error(e.message));
  }
});

app.get('/api/products/:id', async (req, res) => {
  try {
    const r = await pool.query(
      `SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock,
       c.name as category_name, p.created_at
       FROM products p LEFT JOIN categories c ON p.category_id = c.id WHERE p.id = $1`,
      [req.params.id]
    );
    if (r.rows.length === 0) return res.status(404).json(error('Product not found'));
    res.json(success(r.rows[0]));
  } catch (e) {
    console.error('GET /api/products/:id:', e);
    res.status(500).json(error(e.message));
  }
});

app.get('/api/products/category/:categoryName', async (req, res) => {
  try {
    const r = await pool.query(
      `SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock,
       c.name as category_name, p.created_at
       FROM products p LEFT JOIN categories c ON p.category_id = c.id
       WHERE LOWER(c.name) = LOWER($1) ORDER BY p.id`,
      [req.params.categoryName]
    );
    res.json(success(r.rows));
  } catch (e) {
    console.error('GET /api/products/category:', e);
    res.status(500).json(error(e.message));
  }
});

app.get('/api/products/search', async (req, res) => {
  try {
    const q = req.query.q || '';
    const r = await pool.query(
      `SELECT p.id, p.category_id, p.name, p.description, p.price, p.image_url, p.stock,
       c.name as category_name, p.created_at
       FROM products p LEFT JOIN categories c ON p.category_id = c.id
       WHERE p.name ILIKE $1 OR p.description ILIKE $1 ORDER BY p.id`,
      [`%${q}%`]
    );
    res.json(success(r.rows));
  } catch (e) {
    console.error('GET /api/products/search:', e);
    res.status(500).json(error(e.message));
  }
});

// Auth
app.post('/api/auth/register', async (req, res) => {
  try {
    const { email, password, name } = req.body;
    if (!email || !password || !name) return res.status(400).json(error('Missing email, password, or name'));
    if (password.length < 6) return res.status(400).json(error('Password must be at least 6 characters'));
    const hash = sha256(password);
    const r = await pool.query(
      'INSERT INTO users (email, password_hash, name) VALUES ($1, $2, $3) RETURNING id, email, name, created_at',
      [email, hash, name]
    );
    const u = r.rows[0];
    res.status(201).json(success({ user: { id: u.id, email: u.email, name: u.name, created_at: u.created_at } }));
  } catch (e) {
    if (e.code === '23505') return res.status(409).json(error('Email already registered'));
    console.error('POST /api/auth/register:', e);
    res.status(500).json(error(e.message));
  }
});

app.post('/api/auth/login', async (req, res) => {
  try {
    const { email, password } = req.body;
    if (!email || !password) return res.status(400).json(error('Missing email or password'));
    const hash = sha256(password);
    const r = await pool.query(
      'SELECT id, email, name, created_at FROM users WHERE email = $1 AND password_hash = $2',
      [email, hash]
    );
    if (r.rows.length === 0) return res.status(401).json(error('Invalid email or password'));
    const u = r.rows[0];
    res.json(success({ user: { id: u.id, email: u.email, name: u.name, created_at: u.created_at } }));
  } catch (e) {
    console.error('POST /api/auth/login:', e);
    res.status(500).json(error(e.message));
  }
});

// Cart
app.get('/api/cart/:userId', async (req, res) => {
  try {
    const r = await pool.query(
      `SELECT ci.id, ci.user_id, ci.product_id, ci.quantity, p.name as product_name, p.price, p.image_url
       FROM cart_items ci JOIN products p ON ci.product_id = p.id WHERE ci.user_id = $1`,
      [req.params.userId]
    );
    res.json(success(r.rows));
  } catch (e) {
    console.error('GET /api/cart:', e);
    res.status(500).json(error(e.message));
  }
});

app.post('/api/cart/add', async (req, res) => {
  try {
    const { user_id, product_id, quantity = 1 } = req.body;
    if (!user_id || !product_id) return res.status(400).json(error('Missing user_id or product_id'));
    const qty = Math.max(1, parseInt(quantity, 10) || 1);
    await pool.query(
      `INSERT INTO cart_items (user_id, product_id, quantity) VALUES ($1, $2, $3)
       ON CONFLICT (user_id, product_id) DO UPDATE SET quantity = cart_items.quantity + EXCLUDED.quantity`,
      [user_id, product_id, qty]
    );
    res.status(201).json({ success: true, message: 'Item added to cart' });
  } catch (e) {
    console.error('POST /api/cart/add:', e);
    res.status(500).json(error(e.message));
  }
});

app.post('/api/cart/remove', async (req, res) => {
  try {
    const { user_id, product_id } = req.body;
    if (!user_id || !product_id) return res.status(400).json(error('Missing user_id or product_id'));
    await pool.query('DELETE FROM cart_items WHERE user_id = $1 AND product_id = $2', [user_id, product_id]);
    res.json({ success: true, message: 'Item removed from cart' });
  } catch (e) {
    console.error('POST /api/cart/remove:', e);
    res.status(500).json(error(e.message));
  }
});

app.post('/api/cart/update_quantity', async (req, res) => {
  try {
    const { user_id, product_id, quantity } = req.body;
    if (!user_id || !product_id || quantity == null) return res.status(400).json(error('Missing user_id, product_id, or quantity'));
    const qty = parseInt(quantity, 10);
    if (qty < 1) return res.status(400).json(error('Quantity must be at least 1'));
    await pool.query('UPDATE cart_items SET quantity = $1 WHERE user_id = $2 AND product_id = $3', [qty, user_id, product_id]);
    res.json({ success: true, message: 'Cart updated' });
  } catch (e) {
    console.error('POST /api/cart/update_quantity:', e);
    res.status(500).json(error(e.message));
  }
});

// Orders
app.post('/api/orders/create', async (req, res) => {
  const client = await pool.connect();
  try {
    const { user_id, items, shipping_name, shipping_phone, shipping_address, payment_method } = req.body;
    if (!user_id || !items || !Array.isArray(items) || items.length === 0) {
      return res.status(400).json(error('Missing user_id or items'));
    }
    let total = 0;
    for (const it of items) {
      const pid = it.product_id;
      const qty = parseInt(it.quantity, 10) || 0;
      if (qty < 1) continue;
      const pr = await client.query('SELECT price, stock FROM products WHERE id = $1', [pid]);
      if (pr.rows.length === 0) return res.status(400).json(error('Product not found: ' + pid));
      const { price, stock } = pr.rows[0];
      if (qty > stock) return res.status(400).json(error('Insufficient stock for product ' + pid));
      total += parseFloat(price) * qty;
    }
    const orderR = await client.query(
      `INSERT INTO orders (user_id, total, status, shipping_name, shipping_phone, shipping_address, payment_method)
       VALUES ($1, $2, $3, $4, $5, $6, $7) RETURNING id, total`,
      [user_id, total, 'pending', shipping_name || null, shipping_phone || null, shipping_address || null, payment_method || null]
    );
    const orderId = orderR.rows[0].id;
    for (const it of items) {
      const pid = it.product_id;
      const qty = parseInt(it.quantity, 10) || 0;
      if (qty < 1) continue;
      const pr = await client.query('SELECT price FROM products WHERE id = $1', [pid]);
      const price = parseFloat(pr.rows[0].price);
      await client.query(
        'INSERT INTO order_items (order_id, product_id, quantity, price_at_purchase) VALUES ($1, $2, $3, $4)',
        [orderId, pid, qty, price]
      );
      await client.query('UPDATE products SET stock = stock - $1 WHERE id = $2', [qty, pid]);
    }
    await client.query('DELETE FROM cart_items WHERE user_id = $1', [user_id]);
    res.status(201).json(success({ order_id: orderId, total }));
  } catch (e) {
    console.error('POST /api/orders/create:', e);
    res.status(500).json(error(e.message));
  } finally {
    client.release();
  }
});

app.get('/api/orders/:userId', async (req, res) => {
  try {
    const orders = await pool.query(
      'SELECT id, user_id, total, status, created_at FROM orders WHERE user_id = $1 ORDER BY created_at DESC',
      [req.params.userId]
    );
    const result = [];
    for (const o of orders.rows) {
      const items = await pool.query(
        `SELECT oi.product_id, p.name as product_name, oi.quantity, oi.price_at_purchase
         FROM order_items oi JOIN products p ON oi.product_id = p.id WHERE oi.order_id = $1`,
        [o.id]
      );
      result.push({
        id: o.id,
        user_id: o.user_id,
        total: parseFloat(o.total),
        status: o.status,
        created_at: o.created_at,
        items: items.rows.map((i) => ({
          product_id: i.product_id,
          product_name: i.product_name,
          quantity: i.quantity,
          price_at_purchase: parseFloat(i.price_at_purchase),
        })),
      });
    }
    res.json(success(result));
  } catch (e) {
    console.error('GET /api/orders:', e);
    res.status(500).json(error(e.message));
  }
});

// Health check
app.get('/api/health', async (req, res) => {
  try {
    await pool.query('SELECT 1');
    res.json({ ok: true, db: 'connected' });
  } catch (e) {
    res.status(500).json({ ok: false, db: 'error', error: e.message });
  }
});

const PORT = parseInt(process.env.PORT || '8080', 10);

async function start() {
  try {
    await pool.query('SELECT 1');
    console.log('Database connected successfully');
  } catch (e) {
    console.error('Database connection failed:', e.message);
    console.error('Check that PostgreSQL is running and db config is correct.');
    process.exit(1);
  }
  app.listen(PORT, '0.0.0.0', () => {
    console.log(`Lala Store API running at http://localhost:${PORT}`);
  });
}

start().catch((e) => {
  console.error('Startup error:', e);
  process.exit(1);
});
