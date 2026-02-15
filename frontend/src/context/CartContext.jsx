import { createContext, useContext, useState, useEffect } from 'react';
import { cartApi } from '../api/api';

const CartContext = createContext(null);

export function CartProvider({ children }) {
  const [cart, setCart] = useState([]);
  const [userId, setUserId] = useState(() => {
    const u = localStorage.getItem('user');
    if (u) {
      try {
        const parsed = JSON.parse(u);
        return parsed?.id ?? null;
      } catch {
        return null;
      }
    }
    return null;
  });

  const refreshCart = async () => {
    if (!userId) {
      setCart([]);
      return;
    }
    try {
      const res = await cartApi.get(userId);
      if (res.data?.success && res.data?.data) {
        setCart(Array.isArray(res.data.data) ? res.data.data : []);
      } else {
        setCart([]);
      }
    } catch {
      setCart([]);
    }
  };

  useEffect(() => {
    refreshCart();
  }, [userId]);

  const addToCart = async (productId, quantity = 1) => {
    if (!userId) return { ok: false, error: 'Please log in to add to cart' };
    try {
      await cartApi.add(userId, productId, quantity);
      await refreshCart();
      return { ok: true };
    } catch (err) {
      return { ok: false, error: err.response?.data?.error || 'Failed to add to cart' };
    }
  };

  const removeFromCart = async (productId) => {
    if (!userId) return;
    try {
      await cartApi.remove(userId, productId);
      await refreshCart();
    } catch {}
  };

  const updateQuantity = async (productId, quantity) => {
    if (!userId) return;
    if (quantity < 1) return removeFromCart(productId);
    try {
      await cartApi.updateQuantity(userId, productId, quantity);
      await refreshCart();
    } catch {}
  };

  const setUser = (user) => {
    if (user) {
      localStorage.setItem('user', JSON.stringify(user));
      setUserId(user.id);
    } else {
      localStorage.removeItem('user');
      setUserId(null);
      setCart([]);
    }
  };

  const cartCount = cart.reduce((sum, item) => sum + item.quantity, 0);

  return (
    <CartContext.Provider
      value={{
        cart,
        cartCount,
        userId,
        setUser,
        refreshCart,
        addToCart,
        removeFromCart,
        updateQuantity,
      }}
    >
      {children}
    </CartContext.Provider>
  );
}

export function useCart() {
  const ctx = useContext(CartContext);
  if (!ctx) throw new Error('useCart must be used within CartProvider');
  return ctx;
}
