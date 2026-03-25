document.addEventListener("DOMContentLoaded", () => {
  if (window.PortalState.token) {
    window.PortalAuth.checkAuth();
  }

  document.getElementById("loginForm")?.addEventListener("submit", async (e) => {
    e.preventDefault();
    const username = document.getElementById("loginUsername").value;
    const password = document.getElementById("loginPassword").value;

    try {
      await window.PortalAuth.login(username, password);
    } catch (error) {
      window.PortalUI.showNotification(error.message, "error", 5000);
    }
  });

  document.getElementById("registerForm")?.addEventListener("submit", async (e) => {
    e.preventDefault();
    const username = document.getElementById("regUsername").value.trim();
    const email = document.getElementById("regEmail").value.trim();
    const password = document.getElementById("regPassword").value;
    const passwordConfirm = document.getElementById("regPasswordConfirm").value;

    if (password !== passwordConfirm) {
      window.PortalUI.showNotification("两次输入的密码不一致", "warning", 5000);
      return;
    }

    try {
      await window.PortalAuth.register({ username, email, password });
      window.PortalUI.showNotification("注册成功！正在跳转到登录页面...", "success", 2500);
      setTimeout(() => window.PortalUI.showLogin(), 2000);
    } catch (error) {
      window.PortalUI.showNotification("注册失败：" + error.message, "error", 5000);
    }
  });

  document.getElementById("changePasswordForm")?.addEventListener("submit", async (e) => {
    e.preventDefault();
    const oldPassword = document.getElementById("oldPassword").value;
    const newPassword = document.getElementById("newPassword").value;
    const newPasswordConfirm = document.getElementById("newPasswordConfirm").value;

    if (newPassword !== newPasswordConfirm) {
      window.PortalProfile.showPasswordError("两次输入的新密码不一致");
      return;
    }

    try {
      await window.PortalProfile.changePassword(oldPassword, newPassword);
      window.PortalProfile.showPasswordSuccess("密码修改成功！");
      document.getElementById("changePasswordForm").reset();
    } catch (error) {
      window.PortalProfile.showPasswordError("修改失败：" + error.message);
    }
  });

  document.getElementById("updateProfileForm")?.addEventListener("submit", async (e) => {
    e.preventDefault();
    try {
      await window.PortalProfile.updateProfile({
        email: document.getElementById("settingsEmail").value.trim(),
        full_name: document.getElementById("settingsFullName").value.trim(),
        company: document.getElementById("settingsCompany").value.trim(),
      });
      window.PortalProfile.showProfileSuccess("基本信息保存成功！");
    } catch (error) {
      window.PortalProfile.showProfileError("保存失败：" + error.message);
    }
  });

  document.getElementById("applyForm")?.addEventListener("submit", async (e) => {
    e.preventDefault();
    await window.PortalApply.submit();
  });
});
