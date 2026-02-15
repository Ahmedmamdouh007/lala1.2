import { useState, useEffect } from 'react';
import { useParams, Link } from 'react-router-dom';
import { productsApi } from '../api/api';
import { useCart } from '../context/CartContext';
import './ProductDetails.css';

export default function ProductDetails() {
  const { id } = useParams();
  const { addToCart, userId } = useCart();
  const [product, setProduct] = useState(null);
  const [loading, setLoading] = useState(true);
  const [quantity, setQuantity] = useState(1);
  const [message, setMessage] = useState(null);

  useEffect(() => {
    productsApi.getById(id)
      .then((res) => {
        if (res.data?.success && res.data?.data) {
          setProduct(res.data.data);
        } else {
          setProduct(null);
        }
      })
      .catch(() => setProduct(null))
      .finally(() => setLoading(false));
  }, [id]);

  const handleAddToCart = async () => {
    if (!product?.id) return;
    const result = await addToCart(product.id, quantity);
    if (result.ok) {
      setMessage('Added to cart!');
      setTimeout(() => setMessage(null), 2000);
    } else {
      setMessage(result.error || 'Failed');
    }
  };

  if (loading) {
    return <div className="loading">Loading product...</div>;
  }

  if (!product) {
    return (
      <div className="product-not-found">
        <h2>Product not found</h2>
        <Link to="/">Go back to home</Link>
      </div>
    );
  }

  return (
    <div className="product-details-page">
      <div className="product-details-content">
        <div className="product-image-section">
          <img
            src={product.image_url || 'https://via.placeholder.com/500x600?text=No+Image'}
            alt={product.name}
            onError={(e) => {
              e.target.src = 'https://via.placeholder.com/500x600?text=No+Image';
            }}
          />
        </div>
        <div className="product-info-section">
          <h1>{product.name}</h1>
          <p className="product-category">{product.category_name || 'Uncategorized'}</p>
          <p className="product-price">${Number(product.price ?? 0).toFixed(2)}</p>
          <p className="product-description">{product.description || ''}</p>
          <div className="product-meta">
            <p><strong>Stock:</strong> {product.stock ?? 0} available</p>
          </div>
          <div className="product-actions">
            <div className="quantity-selector">
              <label>Quantity:</label>
              <button type="button" onClick={() => setQuantity(Math.max(1, quantity - 1))}>-</button>
              <span>{quantity}</span>
              <button type="button" onClick={() => setQuantity(Math.min(product.stock ?? 999, quantity + 1))}>+</button>
            </div>
            <button
              type="button"
              className="add-to-cart-btn-large"
              onClick={handleAddToCart}
              disabled={!userId || (product.stock ?? 0) === 0}
            >
              {(product.stock ?? 0) === 0 ? 'Out of Stock' : 'Add to Cart'}
            </button>
          </div>
          {message && (
            <p className={`product-message ${message.includes('Added') ? 'success' : 'error'}`}>
              {message}
            </p>
          )}
          {!userId && <p className="product-message" style={{ color: 'var(--muted)', fontSize: '0.9rem' }}>Log in to add to cart</p>}
        </div>
      </div>
    </div>
  );
}
