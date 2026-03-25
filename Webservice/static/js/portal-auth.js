const PortalAuth = {
  async parseError(response, fallbackMessage) {
    try {
      const error = await response.json();
      return error?.detail || fallbackMessage;
    } catch (_) {
      return fallbackMessage;
    }
  },

  async handleAuthFailure(response, fallbackMessage = "登录状态已失效") {
    const message = await this.parseError(response, fallbackMessage);
    if (response.status === 403 && message.includes("封禁")) {
      this.logout();
      window.PortalUI?.showAlert("当前账号已被封禁", "账号状态异常", "warning");
      throw new Error("当前账号已被封禁");
    }
    if (response.status === 401) {
      this.logout();
      throw new Error("登录状态已失效，请重新登录");
    }
    throw new Error(message);
  },

  async login(username, password) {
    const response = await fetch(`${window.PortalConfig.API_BASE}/auth/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, password }),
    });

    if (!response.ok) {
      throw new Error(await this.parseError(response, "用户名或密码错误"));
    }

    const data = await response.json();
    window.PortalState.setToken(data.access_token);
    await this.checkAuth();
  },

  async register({ username, email, password }) {
    const response = await fetch(`${window.PortalConfig.API_BASE}/auth/register`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ username, email, password }),
    });

    if (!response.ok) {
      const error = await response.json();
      throw new Error(error.detail || "未知错误");
    }
  },

  async checkAuth() {
    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/auth/me`, {
        headers: { Authorization: `Bearer ${window.PortalState.token}` },
      });

      if (!response.ok) {
        await this.handleAuthFailure(response, "登录状态已失效");
        return;
      }

      window.PortalState.currentUser = await response.json();
      window.PortalUI.showDashboard();
      window.PortalProfile.loadUserProfile();
    } catch (error) {
      if (error.message !== "当前账号已被封禁") {
        this.logout();
      }
    }
  },

  logout() {
    window.PortalState.clear();
    document.getElementById("dashboardPage").classList.add("hidden");
    window.PortalUI.showLogin();
  },
};

window.logout = () => PortalAuth.logout();
window.PortalAuth = PortalAuth;
