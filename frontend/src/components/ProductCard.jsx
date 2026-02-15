import { Link } from 'react-router-dom';
import { useCart } from '../context/CartContext';
import './ProductCard.css';

export default function ProductCard({ product }) {
  const { addToCart, userId } = useCart();

  const handleAddToCart = async (e) => {
    e.preventDefault();
    e.stopPropagation();
    if (!product?.id) return;
    await addToCart(product.id, 1);
  };

  return (
    <div className="product-card">
      <Link to={`/product/${product.id}`} className="product-link">
        <div className="product-image-container">
          <img
            src={product.image_url || 'https://via.placeholder.com/300x400?text=No+Image'}
            alt={product.name}
            className="product-image"
            onError={(e) => {
              e.target.src = 'https://via.placeholder.com/300x400?text=No+Image';
            }}
          />
        </div>
        <div className="product-info">
          <h3 className="product-name">{product.name}</h3>
          <p className="product-description">{product.description || ''}</p>
          <div className="product-footer">
            <span className="product-price">${Number(product.price ?? 0).toFixed(2)}</span>
            <button
              className="add-to-cart-btn"
              onClick={handleAddToCart}
              disabled={!userId}
            >
              Add to Cart
            </button>
          </div>
        </div>
      </Link>
    </div>
  );
}
