import axios from 'axios';

const api = axios.create({
  baseURL: '/api',
  headers: {
    'Content-Type': 'application/json',
  },
});

export const productsApi = {
  getAll: () => api.get('/products'),
  getById: (id) => api.get(`/products/${id}`),
  getByCategory: (categoryName) => api.get(`/products/category/${categoryName}`),
  search: (query) => api.get('/products/search', { params: { q: query } }),
};

export const authApi = {
  register: (email, password, name) => api.post('/auth/register', { email, password, name }),
  login: (email, password) => api.post('/auth/login', { email, password }),
};

export const cartApi = {
  get: (userId) => api.get(`/cart/${userId}`),
  add: (userId, productId, quantity = 1) => api.post('/cart/add', { user_id: userId, product_id: productId, quantity }),
  remove: (userId, productId) => api.post('/cart/remove', { user_id: userId, product_id: productId }),
  updateQuantity: (userId, productId, quantity) => api.post('/cart/update_quantity', { user_id: userId, product_id: productId, quantity }),
};

export const ordersApi = {
  create: (userId, items, shipping = {}) => api.post('/orders/create', { user_id: userId, items, ...shipping }),
  getByUser: (userId) => api.get(`/orders/${userId}`),
};

export default api;
