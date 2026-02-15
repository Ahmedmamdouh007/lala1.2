import { useState, useEffect } from 'react';
import { productsApi } from '../api/api';
import ProductCard from '../components/ProductCard';
import './Men.css';

export default function Men() {
  const [products, setProducts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const loadProducts = async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await productsApi.getByCategory('Men');
      if (res.data?.success && res.data?.data) {
        setProducts(Array.isArray(res.data.data) ? res.data.data : []);
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
    loadProducts();
  }, []);

  if (loading) {
    return (
      <div className="men-page">
        <div className="page-header">
          <h1>Men&apos;s Collection</h1>
          <p>Discover our latest men&apos;s fashion</p>
        </div>
        <div className="loading-skeleton">
          {[...Array(8)].map((_, i) => (
            <div key={i} className="skeleton-card">
              <div className="skeleton-image" />
              <div className="skeleton-text" />
              <div className="skeleton-text short" />
            </div>
          ))}
        </div>
      </div>
    );
  }

  return (
    <div className="men-page">
      <div className="page-header">
        <h1>Men&apos;s Collection</h1>
        <p>Discover our latest men&apos;s fashion</p>
      </div>
      {products.length === 0 ? (
        <p className="no-products">
          {error || "No men's products available at the moment."}
          {error && <button type="button" className="retry-btn" onClick={loadProducts} style={{ marginLeft: '0.5rem' }}>Retry</button>}
        </p>
      ) : (
        <div className="products-grid">
          {products.map((p) => (
            <ProductCard key={p.id} product={p} />
          ))}
        </div>
      )}
    </div>
  );
}
