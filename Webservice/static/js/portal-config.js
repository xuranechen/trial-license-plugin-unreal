// Portal 配置文件
const API_BASE = "http://localhost:8000/api/v1";

// 导出配置
window.PortalConfig = {
  API_BASE,
  token: localStorage.getItem("user_token"),
  currentUser: null,
  
  setToken(token) {
    this.token = token;
    if (token) {
      localStorage.setItem("user_token", token);
    } else {
      localStorage.removeItem("user_token");
    }
  },
  
  getToken() {
    return this.token || localStorage.getItem("user_token");
  }
};
