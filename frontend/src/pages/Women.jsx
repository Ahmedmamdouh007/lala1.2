import { useState, useEffect } from 'react';
import { productsApi } from '../api/api';
import ProductCard from '../components/ProductCard';
import './Women.css';

export default function Women() {
  const [products, setProducts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const loadProducts = async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await productsApi.getByCategory('Women');
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
      <div className="women-page">
        <div className="page-header">
          <h1>Women&apos;s Collection</h1>
          <p>Discover our latest women&apos;s fashion</p>
        </div>
        <div className="loading">Loading...</div>
      </div>
    );
  }

  return (
    <div className="women-page">
      <div className="page-header">
        <h1>Women&apos;s Collection</h1>
        <p>Discover our latest women&apos;s fashion</p>
      </div>
      {products.length === 0 ? (
        <p className="no-products">
          {error || "No women's products available at the moment."}
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
