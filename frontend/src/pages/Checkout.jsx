import { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { ordersApi } from '../api/api';
import { useCart } from '../context/CartContext';
import './Checkout.css';

const PAYMENT_OPTIONS = [
  { value: '', label: 'Select payment method' },
  { value: 'cod', label: 'Cash on Delivery' },
  { value: 'visa', label: 'Visa Card' },
  { value: 'mastercard', label: 'Mastercard' },
  { value: 'paypal', label: 'PayPal' },
  { value: 'bank', label: 'Bank Transfer' },
];

// Visa card number validation: 13 or 16 digits, starts with 4
function isValidVisaNumber(num) {
  const cleaned = num.replace(/\s/g, '');
  return /^4\d{12}(?:\d{3})?$/.test(cleaned);
}

// Format card number as XXXX XXXX XXXX XXXX
function formatCardNumber(value) {
  const v = value.replace(/\D/g, '').slice(0, 16);
  return v.replace(/(\d{4})(?=\d)/g, '$1 ');
}

// Format expiry as MM/YY
function formatExpiry(value) {
  const v = value.replace(/\D/g, '').slice(0, 4);
  if (v.length >= 2) {
    return v.slice(0, 2) + '/' + v.slice(2);
  }
  return v;
}

export default function Checkout() {
  const navigate = useNavigate();
  const { cart, userId, refreshCart } = useCart();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [name, setName] = useState('');
  const [phone, setPhone] = useState('');
  const [address, setAddress] = useState('');
  const [paymentMethod, setPaymentMethod] = useState('');
  const [cardNumber, setCardNumber] = useState('');
  const [cardExpiry, setCardExpiry] = useState('');
  const [cardCvv, setCardCvv] = useState('');
  const [cardholderName, setCardholderName] = useState('');

  const total = cart.reduce((sum, item) => sum + item.price * item.quantity, 0);
  const isVisa = paymentMethod === 'visa';

  const validateForm = () => {
    if (!name.trim()) return 'Please enter your name';
    if (!phone.trim()) return 'Please enter your phone number';
    if (!address.trim()) return 'Please enter your shipping address';
    if (!paymentMethod) return 'Please select a payment method';
    if (isVisa) {
      if (!isValidVisaNumber(cardNumber)) return 'Please enter a valid Visa card number (16 digits, starts with 4)';
      if (!/^(0[1-9]|1[0-2])\/\d{2}$/.test(cardExpiry)) return 'Please enter a valid expiry date (MM/YY)';
      if (!/^\d{3,4}$/.test(cardCvv)) return 'Please enter a valid CVV (3-4 digits)';
      if (!cardholderName.trim()) return 'Please enter the cardholder name';
    }
    return null;
  };

  const handlePlaceOrder = async () => {
    if (!userId || cart.length === 0) return;
    const validationError = validateForm();
    if (validationError) {
      setError(validationError);
      return;
    }
    setLoading(true);
    setError(null);
    try {
      const items = cart.map((item) => ({
        product_id: item.product_id,
        quantity: item.quantity,
      }));
      const shipping = {
        shipping_name: name.trim(),
        shipping_phone: phone.trim(),
        shipping_address: address.trim(),
        payment_method: paymentMethod,
      };
      const res = await ordersApi.create(userId, items, shipping);
      if (res.data?.success) {
        await refreshCart();
        navigate('/');
      } else {
        setError(res.data?.error || 'Order failed');
      }
    } catch (err) {
      setError(err.response?.data?.error || 'Failed to place order');
    } finally {
      setLoading(false);
    }
  };

  if (!userId) {
    return (
      <div className="checkout-page">
        <h1>Checkout</h1>
        <div className="empty-cart">
          <h2>Please log in to checkout</h2>
          <Link to="/login" className="back-btn" style={{ display: 'inline-block', textDecoration: 'none', marginTop: '1rem' }}>
            Login
          </Link>
        </div>
      </div>
    );
  }

  if (cart.length === 0) {
    return (
      <div className="checkout-page">
        <h1>Checkout</h1>
        <div className="empty-cart">
          <h2>Your cart is empty</h2>
          <button type="button" className="back-btn" onClick={() => navigate('/')}>
            Continue Shopping
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="checkout-page">
      <h1>Checkout</h1>
      <div className="checkout-content">
        <div className="checkout-form-section">
          <h2>Shipping & Payment</h2>
          {error && <div className="checkout-error">{error}</div>}
          <form onSubmit={(e) => { e.preventDefault(); handlePlaceOrder(); }}>
            <div className="form-group">
              <label htmlFor="name">Full Name *</label>
              <input
                id="name"
                type="text"
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="Your full name"
                required
              />
            </div>
            <div className="form-group">
              <label htmlFor="phone">Phone Number *</label>
              <input
                id="phone"
                type="tel"
                value={phone}
                onChange={(e) => setPhone(e.target.value)}
                placeholder="e.g. +1 555 123 4567"
                required
              />
            </div>
            <div className="form-group">
              <label htmlFor="address">Shipping Address *</label>
              <textarea
                id="address"
                value={address}
                onChange={(e) => setAddress(e.target.value)}
                placeholder="Street, City, State, Postal Code"
                rows="3"
                required
              />
            </div>
            <div className="form-group">
              <label htmlFor="payment">Payment Method *</label>
              <select
                id="payment"
                value={paymentMethod}
                onChange={(e) => setPaymentMethod(e.target.value)}
                required
              >
                {PAYMENT_OPTIONS.map((opt) => (
                  <option key={opt.value} value={opt.value}>{opt.label}</option>
                ))}
              </select>
            </div>

            {isVisa && (
              <div className="visa-card-section">
                <h3>Visa Card Details</h3>
                <div className="form-group">
                  <label htmlFor="cardNumber">Card Number *</label>
                  <input
                    id="cardNumber"
                    type="text"
                    value={cardNumber}
                    onChange={(e) => setCardNumber(formatCardNumber(e.target.value))}
                    placeholder="4242 4242 4242 4242"
                    maxLength={19}
                  />
                </div>
                <div className="form-group">
                  <label htmlFor="cardholderName">Cardholder Name *</label>
                  <input
                    id="cardholderName"
                    type="text"
                    value={cardholderName}
                    onChange={(e) => setCardholderName(e.target.value)}
                    placeholder="Name on card"
                  />
                </div>
                <div className="visa-row">
                  <div className="form-group">
                    <label htmlFor="expiry">Expiry (MM/YY) *</label>
                    <input
                      id="expiry"
                      type="text"
                      value={cardExpiry}
                      onChange={(e) => setCardExpiry(formatExpiry(e.target.value))}
                      placeholder="MM/YY"
                      maxLength={5}
                    />
                  </div>
                  <div className="form-group">
                    <label htmlFor="cvv">CVV *</label>
                    <input
                      id="cvv"
                      type="password"
                      value={cardCvv}
                      onChange={(e) => setCardCvv(e.target.value.replace(/\D/g, '').slice(0, 4))}
                      placeholder="123"
                      maxLength={4}
                    />
                  </div>
                </div>
              </div>
            )}

            <button
              type="submit"
              className="place-order-btn"
              disabled={loading}
            >
              {loading ? 'Placing Order...' : 'Place Order'}
            </button>
          </form>
        </div>
        <div className="order-summary-section">
          <h2>Order Summary</h2>
          <div className="order-items">
            {cart.map((item) => (
              <div key={item.id} className="order-item">
                <span>{item.product_name || 'Product'} x{item.quantity ?? 1}</span>
                <span>${((item.price ?? 0) * (item.quantity ?? 1)).toFixed(2)}</span>
              </div>
            ))}
          </div>
          <div className="order-total">
            <div className="total-row">
              <span>Subtotal:</span>
              <span>${total.toFixed(2)}</span>
            </div>
            <div className="total-row">
              <span>Shipping:</span>
              <span>Free</span>
            </div>
            <div className="total-row final-total">
              <span>Total:</span>
              <span>${total.toFixed(2)}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
