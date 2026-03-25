const PortalProfile = {
  loadUserProfile() {
    const currentUser = window.PortalState.currentUser;
    if (!currentUser) return;

    document.getElementById("profileUsername").textContent = currentUser.username;
    document.getElementById("profileEmail").textContent = currentUser.email;
    document.getElementById("profileFullName").textContent = currentUser.full_name || "未设置";
    document.getElementById("profileCompany").textContent = currentUser.company || "未设置";
    document.getElementById("profileCreatedAt").textContent = new Date(currentUser.created_at).toLocaleString("zh-CN");
    document.getElementById("profileStatus").textContent = currentUser.is_active ? "正常" : "已禁用";

    const settingsUsername = document.getElementById("settingsUsername");
    const settingsEmail = document.getElementById("settingsEmail");
    const settingsFullName = document.getElementById("settingsFullName");
    const settingsCompany = document.getElementById("settingsCompany");
    if (settingsUsername) settingsUsername.value = currentUser.username || "";
    if (settingsEmail) settingsEmail.value = currentUser.email || "";
    if (settingsFullName) settingsFullName.value = currentUser.full_name || "";
    if (settingsCompany) settingsCompany.value = currentUser.company || "";
  },

  async updateProfile(payload) {
    const response = await fetch(`${window.PortalConfig.API_BASE}/auth/update-profile`, {
      method: "PUT",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${window.PortalState.token}`,
      },
      body: JSON.stringify(payload),
    });

    if (!response.ok) {
      if (response.status === 401 || response.status === 403) {
        await window.PortalAuth.handleAuthFailure(response, "基本信息更新失败");
        return null;
      }
      const error = await response.json();
      throw new Error(error.detail || "未知错误");
    }

    const updatedUser = await response.json();
    window.PortalState.currentUser = updatedUser;
    window.PortalUI.showDashboard();
    this.loadUserProfile();
    return updatedUser;
  },

  async changePassword(oldPassword, newPassword) {
    const response = await fetch(`${window.PortalConfig.API_BASE}/auth/change-password`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        Authorization: `Bearer ${window.PortalState.token}`,
      },
      body: JSON.stringify({
        old_password: oldPassword,
        new_password: newPassword,
      }),
    });

    if (!response.ok) {
      if (response.status === 401 || response.status === 403) {
        await window.PortalAuth.handleAuthFailure(response, "密码修改失败");
        return;
      }
      const error = await response.json();
      throw new Error(error.detail || "未知错误");
    }
  },

  showPasswordError(message) {
    window.PortalUI.showNotification(message, "error", 5000);
  },

  showPasswordSuccess(message) {
    window.PortalUI.showNotification(message, "success", 3000);
  },

  showProfileError(message) {
    window.PortalUI.showNotification(message, "error", 5000);
  },

  showProfileSuccess(message) {
    window.PortalUI.showNotification(message, "success", 3000);
  },
};

window.PortalProfile = PortalProfile;
