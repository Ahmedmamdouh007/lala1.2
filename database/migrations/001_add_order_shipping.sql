-- Add shipping and payment columns to orders (for existing databases)
ALTER TABLE orders ADD COLUMN IF NOT EXISTS shipping_name VARCHAR(255);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS shipping_phone VARCHAR(50);
ALTER TABLE orders ADD COLUMN IF NOT EXISTS shipping_address TEXT;
ALTER TABLE orders ADD COLUMN IF NOT EXISTS payment_method VARCHAR(50);
