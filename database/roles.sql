-- Database roles for Lala Store
-- Run after schema (docker-entrypoint-initdb.d/02-roles.sql)
-- Creates: app_user (full app access), lab_readonly (SELECT only on products/categories)

-- Application role: normal permissions for the app (all tables)
CREATE ROLE app_user WITH LOGIN PASSWORD 'app_pass';

GRANT CONNECT ON DATABASE lala_store TO app_user;
GRANT USAGE ON SCHEMA public TO app_user;
GRANT SELECT, INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA public TO app_user;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO app_user;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT, INSERT, UPDATE, DELETE ON TABLES TO app_user;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT USAGE, SELECT ON SEQUENCES TO app_user;

-- Lab role: SELECT only on products and categories (no users, cart, orders)
CREATE ROLE lab_readonly WITH LOGIN PASSWORD 'lab_readonly_pass';

GRANT CONNECT ON DATABASE lala_store TO lab_readonly;
GRANT USAGE ON SCHEMA public TO lab_readonly;
GRANT SELECT ON products TO lab_readonly;
GRANT SELECT ON categories TO lab_readonly;
