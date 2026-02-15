-- Lala Store Seed Data
-- Run after schema.sql via docker-entrypoint-initdb.d

-- Categories
INSERT INTO categories (name) VALUES ('Men'), ('Women') ON CONFLICT (name) DO NOTHING;

-- Men's Products (12 items)
INSERT INTO products (category_id, name, description, price, image_url, stock) VALUES
(1, 'Classic White T-Shirt', 'Premium cotton crew neck t-shirt. Comfortable fit for everyday wear.', 24.99, 'https://images.unsplash.com/photo-1521572163474-6864f9cf17ab?w=400', 50),
(1, 'Slim Fit Chinos', 'Versatile slim fit chinos in navy. Perfect for smart casual occasions.', 59.99, 'https://images.unsplash.com/photo-1624378439575-d8705ad7ae80?w=400', 30),
(1, 'Leather Jacket', 'Genuine leather moto jacket. Timeless style with a modern edge.', 199.99, 'https://images.unsplash.com/photo-1551028719-00167b16eac5?w=400', 15),
(1, 'Wool Blend Sweater', 'Soft wool blend crew neck sweater. Ideal for layering in cooler weather.', 79.99, 'https://images.unsplash.com/photo-1576566588028-4147f3842f27?w=400', 25),
(1, 'Oxford Dress Shirt', 'Classic oxford cloth button-down. Crisp white for formal occasions.', 49.99, 'https://images.unsplash.com/photo-1596755094514-f87e34085b2c?w=400', 40),
(1, 'Denim Jeans - Dark Wash', 'Premium denim with dark wash finish. Straight fit for all-day comfort.', 69.99, 'https://images.unsplash.com/photo-1542272604-787c3835535d?w=400', 35),
(1, 'Hooded Sweatshirt', 'Cozy cotton hoodie with kangaroo pocket. Street-ready style.', 44.99, 'https://images.unsplash.com/photo-1556821840-3a63f95609a7?w=400', 45),
(1, 'Tailored Blazer', 'Slim fit blazer in charcoal. Perfect for office or evening wear.', 149.99, 'https://images.unsplash.com/photo-1507679799987-c73779587ccf?w=400', 20),
(1, 'Polo Shirt', 'Classic polo in navy blue. Breathable pique cotton.', 39.99, 'https://images.unsplash.com/photo-1586790170083-2f9ceadc732d?w=400', 55),
(1, 'Cargo Pants', 'Utility cargo pants with multiple pockets. Durable cotton twill.', 54.99, 'https://images.unsplash.com/photo-1624378439575-d8705ad7ae80?w=400', 28),
(1, 'Winter Parka', 'Insulated parka with faux fur hood. Weather-resistant outer shell.', 249.99, 'https://images.unsplash.com/photo-1539533018447-63fcce2678e3?w=400', 12),
(1, 'Running Shorts', 'Lightweight running shorts with built-in brief. Quick-dry fabric.', 29.99, 'https://images.unsplash.com/photo-1591195853828-11db59a44f6b?w=400', 60);

-- Women's Products (12 items)
INSERT INTO products (category_id, name, description, price, image_url, stock) VALUES
(2, 'Floral Summer Dress', 'Flowing midi dress with vibrant floral print. Perfect for warm days.', 89.99, 'https://images.unsplash.com/photo-1595777457583-95e059d581b8?w=400', 30),
(2, 'High-Waist Skinny Jeans', 'Stretch denim skinny jeans. High waist for a flattering fit.', 64.99, 'https://images.unsplash.com/photo-1541099649105-f69ad21f3246?w=400', 40),
(2, 'Cashmere Cardigan', 'Luxurious cashmere blend cardigan. Soft and elegant.', 129.99, 'https://images.unsplash.com/photo-1434389677669-e08b4cac3105?w=400', 18),
(2, 'Silk Blouse', 'Elegant silk blouse with subtle sheen. Timeless wardrobe staple.', 79.99, 'https://images.unsplash.com/photo-1564257631407-4deb1f99d992?w=400', 22),
(2, 'Wide Leg Trousers', 'Sophisticated wide leg trousers. Flowy and comfortable.', 74.99, 'https://images.unsplash.com/photo-1594938298603-c8148c4dae35?w=400', 25),
(2, 'Knit Sweater Dress', 'Cozy ribbed knit sweater dress. Casual yet chic.', 69.99, 'https://images.unsplash.com/photo-1572804013309-59a88b7e92f1?w=400', 20),
(2, 'Leather Crossbody Bag', 'Compact leather crossbody. Perfect for everyday essentials.', 99.99, 'https://images.unsplash.com/photo-1548036328-c9fa89d128fa?w=400', 35),
(2, 'Linen Blazer', 'Lightweight linen blazer in cream. Breathable for warmer months.', 119.99, 'https://images.unsplash.com/photo-1591047139829-d91aecb6caea?w=400', 15),
(2, 'Wrap Top', 'Flattering wrap-style top. Versatile for work or weekend.', 44.99, 'https://images.unsplash.com/photo-1564257631407-4deb1f99d992?w=400', 38),
(2, 'Midi Skirt', 'Pleated midi skirt in neutral tones. Easy to style.', 54.99, 'https://images.unsplash.com/photo-1583496661160-fb5886a08b2f?w=400', 28),
(2, 'Quilted Puffer Jacket', 'Lightweight puffer jacket. Warm without the bulk.', 139.99, 'https://images.unsplash.com/photo-1539533018447-63fcce2678e3?w=400', 20),
(2, 'Yoga Leggings', 'High-waist yoga leggings with pockets. Four-way stretch fabric.', 49.99, 'https://images.unsplash.com/photo-1506629082955-511b1aa562c8?w=400', 50);
