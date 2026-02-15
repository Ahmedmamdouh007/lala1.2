import { Link } from 'react-router-dom';
import { useCart } from '../context/CartContext';
import './CartItem.css';

export default function CartItem({ item }) {
  const { updateQuantity, removeFromCart } = useCart();

  const handleQuantityChange = (delta) => {
    const newQty = item.quantity + delta;
    if (newQty <= 0) {
      removeFromCart(item.product_id);
    } else {
      updateQuantity(item.product_id, newQty);
    }
  };

  return (
    <div className="cart-item">
      <div className="cart-item-image">
        <Link to={`/product/${item.product_id}`} style={{ display: 'block', height: '100%' }}>
        <img
          src={item.image_url || 'https://via.placeholder.com/100x100?text=No+Image'}
          alt={item.product_name}
          onError={(e) => {
            e.target.src = 'https://via.placeholder.com/100x100?text=No+Image';
          }}
        />
        </Link>
      </div>
      <div className="cart-item-details">
        <Link to={`/product/${item.product_id}`} style={{ color: 'inherit', textDecoration: 'none' }}>
          <h3>{item.product_name}</h3>
        </Link>
        <p className="cart-item-price">${Number(item.price ?? 0).toFixed(2)} each</p>
      </div>
      <div className="cart-item-controls">
        <div className="quantity-controls">
          <button
            type="button"
            className="quantity-btn"
            onClick={() => handleQuantityChange(-1)}
          >
            -
          </button>
          <span className="quantity">{item.quantity}</span>
          <button
            type="button"
            className="quantity-btn"
            onClick={() => handleQuantityChange(1)}
          >
            +
          </button>
        </div>
        <div className="cart-item-total">
          ${((item.price ?? 0) * (item.quantity ?? 1)).toFixed(2)}
        </div>
        <button
          type="button"
          className="remove-btn"
          onClick={() => removeFromCart(item.product_id)}
        >
          Remove
        </button>
      </div>
    </div>
  );
}
