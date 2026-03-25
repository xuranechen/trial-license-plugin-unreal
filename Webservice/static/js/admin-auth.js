// 认证相关功能模块
const AdminAuth = {
  currentUser: null,
  
  // 初始化
  init() {
    const token = AdminAPI.token;
    if (token) {
      this.checkAuth();
    }
  },
  
  // 登录
  async login(username, password) {
    try {
      const data = await AdminAPI.post("/auth/admin/login", 
        { username, password }, 
        { auth: false }
      );
      
      AdminAPI.setToken(data.access_token);
      await this.checkAuth();
      return { success: true };
    } catch (error) {
      return { success: false, error: error.message };
    }
  },
  
  // 检查认证状态
  async checkAuth() {
    try {
      const user = await AdminAPI.get("/auth/me");
      
      // 验证是否是管理员
      if (!user.is_admin) {
        throw new Error("此账号无管理员权限");
      }
      
      this.currentUser = user;
      this.onAuthSuccess();
      return true;
    } catch (error) {
      this.logout();
      return false;
    }
  },
  
  // 退出登录
  logout() {
    AdminAPI.setToken(null);
    this.currentUser = null;
    this.onLogout();
  },
  
  // 更新个人信息
  async updateProfile(profileData) {
    try {
      const user = await AdminAPI.put("/auth/update-profile", profileData);
      this.currentUser = user;
      return { success: true, user };
    } catch (error) {
      return { success: false, error: error.message };
    }
  },
  
  // 修改密码
  async changePassword(oldPassword, newPassword) {
    try {
      await AdminAPI.post("/auth/change-password", {
        old_password: oldPassword,
        new_password: newPassword
      });
      return { success: true };
    } catch (error) {
      return { success: false, error: error.message };
    }
  },
  
  // 认证成功回调
  onAuthSuccess() {
    if (this.currentUser) {
      document.getElementById("userDisplay").innerHTML = 
        this.currentUser.username + '<span class="admin-badge">管理员</span>';
      document.getElementById("loginPage").classList.add("hidden");
      document.getElementById("mainPage").classList.remove("hidden");
      
      // 加载初始数据
      AdminDashboard.load();
      AdminApplications.loadList();
    }
  },
  
  // 退出登录回调
  onLogout() {
    document.getElementById("mainPage").classList.add("hidden");
    document.getElementById("loginPage").classList.remove("hidden");
  }
};

// 登录表单处理
document.getElementById("loginForm")?.addEventListener("submit", async (e) => {
  e.preventDefault();
  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;
  
  const result = await AdminAuth.login(username, password);
  
  if (!result.success) {
    AdminUI.showNotification(result.error || "登录失败：用户名或密码错误", "error");
  }
});

// 全局退出登录函数
function logout() {
  AdminAuth.logout();
}

// 显示账户设置模态框
function showProfileModal() {
  if (!AdminAuth.currentUser) return;
  
  const user = AdminAuth.currentUser;
  document.getElementById("profileUsername").value = user.username;
  document.getElementById("profileEmail").value = user.email || "";
  document.getElementById("profileFullName").value = user.full_name || "";
  document.getElementById("profileCompany").value = user.company || "";
  
  document.getElementById("profileError").classList.add("hidden");
  document.getElementById("profileSuccess").classList.add("hidden");
  document.getElementById("passwordForm").reset();
  
  const modal = document.getElementById("profileModal");
  modal.classList.add("show");
  document.body.style.overflow = "hidden";
}

// 关闭账户设置模态框
function closeProfileModal() {
  const modal = document.getElementById("profileModal");
  modal.classList.remove("show");
  document.body.style.overflow = "auto";
}

// 更新个人信息表单
document.getElementById("profileForm")?.addEventListener("submit", async (e) => {
  e.preventDefault();
  
  const updateData = {
    email: document.getElementById("profileEmail").value.trim(),
    full_name: document.getElementById("profileFullName").value.trim(),
    company: document.getElementById("profileCompany").value.trim()
  };
  
  const result = await AdminAuth.updateProfile(updateData);
  
  if (result.success) {
    showProfileSuccess("个人信息更新成功！");
  } else {
    showProfileError(result.error || "更新失败");
  }
});

// 修改密码表单
document.getElementById("passwordForm")?.addEventListener("submit", async (e) => {
  e.preventDefault();
  
  const oldPassword = document.getElementById("oldPassword").value;
  const newPassword = document.getElementById("newPassword").value;
  const confirmPassword = document.getElementById("confirmPassword").value;
  
  if (newPassword !== confirmPassword) {
    showProfileError("两次输入的新密码不一致");
    return;
  }
  
  if (newPassword.length < 6) {
    showProfileError("新密码长度至少需要6个字符");
    return;
  }
  
  const result = await AdminAuth.changePassword(oldPassword, newPassword);
  
  if (result.success) {
    showProfileSuccess("密码修改成功！");
    document.getElementById("passwordForm").reset();
  } else {
    showProfileError(result.error || "密码修改失败");
  }
});

// 显示profile错误
function showProfileError(message) {
  AdminUI.showNotification(message, "error");
}

// 显示profile成功
function showProfileSuccess(message) {
  AdminUI.showNotification(message, "success");
}
