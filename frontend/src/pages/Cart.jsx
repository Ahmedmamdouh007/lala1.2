import { Link } from 'react-router-dom';
import { useCart } from '../context/CartContext';
import CartItem from '../components/CartItem';
import './Cart.css';

export default function Cart() {
  const { cart, userId } = useCart();

  const total = cart.reduce((sum, item) => sum + item.price * item.quantity, 0);

  if (!userId) {
    return (
      <div className="cart-page">
        <h1>Your Cart</h1>
        <div className="empty-cart">
          <p>Please log in to view your cart.</p>
          <Link to="/login" className="continue-shopping-btn">Login</Link>
        </div>
      </div>
    );
  }

  return (
    <div className="cart-page">
      <h1>Shopping Cart</h1>
      {cart.length === 0 ? (
        <div className="empty-cart">
          <p>Your cart is empty</p>
          <div className="cart-empty-buttons">
            <Link to="/men" className="continue-shopping-btn">Shop Men</Link>
            <Link to="/women" className="continue-shopping-btn">Shop Women</Link>
          </div>
        </div>
      ) : (
        <div className="cart-content">
          <div className="cart-main">
            <div className="cart-items">
              {cart.map((item) => (
                <CartItem key={item.id} item={item} />
              ))}
            </div>
          </div>
          <div className="cart-summary">
            <h2>Order Summary</h2>
            <div className="summary-row">
              <span>Subtotal:</span>
              <span>${total.toFixed(2)}</span>
            </div>
            <div className="summary-row">
              <span>Shipping:</span>
              <span>Free</span>
            </div>
            <div className="summary-row total">
              <span>Total:</span>
              <span>${total.toFixed(2)}</span>
            </div>
            <Link to="/checkout" className="checkout-btn">Proceed to Checkout</Link>
          </div>
        </div>
      )}
    </div>
  );
}
