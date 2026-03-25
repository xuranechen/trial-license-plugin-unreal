// API配置和通用请求方法
const AdminAPI = {
  BASE_URL: "http://localhost:8000/api/v1",
  token: localStorage.getItem("admin_token"),
  
  // 设置token
  setToken(token) {
    this.token = token;
    if (token) {
      localStorage.setItem("admin_token", token);
    } else {
      localStorage.removeItem("admin_token");
    }
  },
  
  // 获取请求头
  getHeaders(includeAuth = true) {
    const headers = {
      "Content-Type": "application/json"
    };
    if (includeAuth && this.token) {
      headers["Authorization"] = `Bearer ${this.token}`;
    }
    return headers;
  },
  
  // 通用请求方法
  async request(endpoint, options = {}) {
    const url = `${this.BASE_URL}${endpoint}`;
    const config = {
      ...options,
      headers: {
        ...this.getHeaders(options.auth !== false),
        ...options.headers
      }
    };
    
    try {
      const response = await fetch(url, config);
      
      if (!response.ok) {
        const error = await response.json();
        throw new Error(error.detail || `请求失败: ${response.status}`);
      }
      
      return await response.json();
    } catch (error) {
      console.error("API请求错误:", error);
      throw error;
    }
  },
  
  // GET请求
  async get(endpoint, options = {}) {
    return this.request(endpoint, { ...options, method: "GET" });
  },
  
  // POST请求
  async post(endpoint, data, options = {}) {
    return this.request(endpoint, {
      ...options,
      method: "POST",
      body: JSON.stringify(data)
    });
  },
  
  // PUT请求
  async put(endpoint, data, options = {}) {
    return this.request(endpoint, {
      ...options,
      method: "PUT",
      body: JSON.stringify(data)
    });
  },
  
  // DELETE请求
  async delete(endpoint, options = {}) {
    return this.request(endpoint, { ...options, method: "DELETE" });
  }
};
