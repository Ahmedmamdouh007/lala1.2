import { useState, useEffect } from 'react';
import { Link } from 'react-router-dom';
import { productsApi } from '../api/api';
import ProductCard from '../components/ProductCard';
import './Home.css';

export default function Home() {
  const [products, setProducts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const loadData = async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await productsApi.getAll();
      if (res.data?.success && res.data?.data) {
        const data = res.data.data;
        setProducts(Array.isArray(data) ? data.slice(0, 8) : []);
      } else {
        setProducts([]);
      }
    } catch (err) {
      setError('Failed to load products');
      setProducts([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  if (loading) {
    return <div className="loading">Loading...</div>;
  }

  return (
    <div className="home">
      <section className="hero">
        <h1>Welcome to LALA STORE</h1>
        <p>Discover the latest fashion trends</p>
      </section>

      <section className="featured-section">
        <h2>Featured Products</h2>
        {error && products.length === 0 ? (
          <p className="no-products">
            {error}
            <button type="button" className="retry-btn" onClick={loadData}>Retry</button>
          </p>
        ) : products.length === 0 ? (
          <p className="no-products">
            No products available at the moment.
            <button type="button" className="retry-btn" onClick={loadData}>Retry</button>
          </p>
        ) : (
          <>
            <div className="products-grid">
              {products.map((p) => (
                <ProductCard key={p.id} product={p} />
              ))}
            </div>
            <div className="hero-buttons">
              <Link to="/men" className="btn-primary">Shop Men</Link>
              <Link to="/women" className="btn-secondary">Shop Women</Link>
            </div>
          </>
        )}
      </section>
    </div>
  );
}
