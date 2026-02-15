import { Link } from 'react-router-dom';
import { useCart } from '../context/CartContext';
import './Navbar.css';

export default function Navbar() {
  const { cartCount, userId, setUser } = useCart();

  return (
    <nav className="navbar">
      <div className="navbar-container">
        <Link to="/" className="navbar-logo">
          LALA STORE
        </Link>
        <ul className="navbar-menu">
          <li>
            <Link to="/" className="navbar-link">Home</Link>
          </li>
          <li>
            <Link to="/men" className="navbar-link">Men</Link>
          </li>
          <li>
            <Link to="/women" className="navbar-link">Women</Link>
          </li>
          <li>
            <Link to="/cart" className="navbar-link cart-link">Cart ({cartCount})</Link>
          </li>
          {userId ? (
            <>
              <li>
                <Link to="/checkout" className="navbar-link">Checkout</Link>
              </li>
              <li>
                <button
                  type="button"
                  className="navbar-link btn-logout"
                  onClick={() => setUser(null)}
                  style={{ background: 'none', border: 'none', cursor: 'pointer' }}
                >
                  Logout
                </button>
              </li>
            </>
          ) : (
            <>
              <li>
                <Link to="/login" className="navbar-link">Login</Link>
              </li>
              <li>
                <Link to="/register" className="navbar-link">Register</Link>
              </li>
            </>
          )}
        </ul>
      </div>
    </nav>
  );
}
