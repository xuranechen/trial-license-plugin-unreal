const PortalUI = {
  showDashboard() {
    document.getElementById("loginPage").classList.add("hidden");
    document.getElementById("registerPage").classList.add("hidden");
    document.getElementById("dashboardPage").classList.remove("hidden");

    const currentUser = window.PortalState.currentUser;
    const portalUserDisplay = document.getElementById("portalUserDisplay");
    if (portalUserDisplay && currentUser) {
      portalUserDisplay.textContent = `${currentUser.username} (${currentUser.email})`;
    }
  },

  showLogin() {
    document.getElementById("registerPage").classList.add("hidden");
    document.getElementById("dashboardPage").classList.add("hidden");
    document.getElementById("loginPage").classList.remove("hidden");
    document.getElementById("loginForm").reset();
    this.hideMessages();
  },

  showRegister() {
    document.getElementById("loginPage").classList.add("hidden");
    document.getElementById("registerPage").classList.remove("hidden");
    document.getElementById("registerForm").reset();
    this.hideMessages();
  },

  switchTab(tabName) {
    document.querySelectorAll(".portal-sidebar .tab").forEach((item) => item.classList.remove("active"));

    document.querySelectorAll(".portal-sidebar .tab").forEach((item) => {
      const itemText = item.textContent.trim();
      if (
        (tabName === "profile" && itemText.includes("个人信息")) ||
        (tabName === "licenses" && itemText.includes("我的许可证")) ||
        (tabName === "orders" && itemText.includes("订单记录")) ||
        (tabName === "apply" && itemText.includes("申请许可证")) ||
        (tabName === "settings" && itemText.includes("账号设置"))
      ) {
        item.classList.add("active");
      }
    });

    document.querySelectorAll(".tab-content").forEach((tab) => tab.classList.add("hidden"));
    document.getElementById(`${tabName}Tab`).classList.remove("hidden");

    switch (tabName) {
      case "profile":
        window.PortalProfile.loadUserProfile();
        break;
      case "settings":
        window.PortalProfile.loadUserProfile();
        break;
      case "licenses":
        window.PortalLicenses.loadLicenses();
        break;
      case "orders":
        window.PortalLicenses.loadOrders();
        break;
      case "apply":
        window.PortalApply.loadProjects();
        break;
      default:
        break;
    }
  },

  navigateToApply() {
    this.switchTab("apply");
  },

  showMessage(elementId, message, pairedId = null, autoHideMs = 0) {
    const element = document.getElementById(elementId);
    if (!element) return;
    element.textContent = message;
    element.classList.remove("hidden");
    if (pairedId) {
      document.getElementById(pairedId)?.classList.add("hidden");
    }
    if (autoHideMs > 0) {
      setTimeout(() => element.classList.add("hidden"), autoHideMs);
    }
  },

  showNotification(message, type = "info", duration = 3000) {
    if (window.ModalSystem && typeof window.ModalSystem.notification === "function") {
      window.ModalSystem.notification(message, type, duration);
      return;
    }
    console.log(`[${type.toUpperCase()}] ${message}`);
  },

  hideMessages() {
    document.querySelectorAll(".alert").forEach((msg) => msg.classList.add("hidden"));
  },

  showAlert(message, title, type) {
    if (window.ModalSystem && typeof window.ModalSystem.alert === "function") {
      window.ModalSystem.alert(message, title || "提示", type || "info");
      return;
    }
    console.log(`[${(type || "info").toUpperCase()}] ${title || "提示"}: ${message}`);
  },

  showDetail(title, items, actions = []) {
    if (window.ModalSystem && typeof window.ModalSystem.detail === "function") {
      window.ModalSystem.detail(title || "详情", items || [], actions || []);
      return;
    }
    console.log(`[DETAIL] ${title || "详情"}`, items || []);
  },

  async downloadLicense(id) {
    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/license/download/${id}`, {
        headers: {
          Authorization: `Bearer ${window.PortalState.token}`,
        },
      });

      if (!response.ok) {
        if (response.status === 401 || response.status === 403) {
          await window.PortalAuth.handleAuthFailure(response, "下载许可证失败");
          return;
        }

        let message = "下载许可证失败";
        try {
          const error = await response.json();
          message = error?.detail || message;
        } catch (_) {}
        this.showNotification(message, "error", 4000);
        return;
      }

      const blob = await response.blob();
      const contentDisposition = response.headers.get("Content-Disposition") || "";
      const filenameMatch = contentDisposition.match(/filename=([^;]+)/i);
      const filename = filenameMatch ? filenameMatch[1].replace(/"/g, "") : `license_${id}.lic`;

      const url = window.URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = filename;
      document.body.appendChild(link);
      link.click();
      link.remove();
      window.URL.revokeObjectURL(url);

      this.showNotification("许可证文件已开始下载", "success", 2500);
    } catch (error) {
      this.showNotification(error.message || "下载许可证失败", "error", 4000);
    }
  },

  viewLicenseDetail(id) {
    this.showAlert("许可证详情功能正在开发中", "提示", "info");
  },

  viewOrderDetail(id) {
    this.showAlert("订单详情功能正在开发中", "提示", "info");
  },

  downloadInvoice(id) {
    this.showAlert("发票下载功能正在开发中", "提示", "info");
  },

  copyLicenseKey(licenseKey) {
    const tempInput = document.createElement("textarea");
    tempInput.value = licenseKey;
    tempInput.style.position = "fixed";
    tempInput.style.opacity = "0";
    document.body.appendChild(tempInput);
    tempInput.select();
    tempInput.setSelectionRange(0, 99999);

    try {
      document.execCommand("copy");
      if (typeof window.showNotification === "function") {
        window.showNotification("许可证密钥已复制到剪贴板", "success", 2200);
      }
    } catch (err) {
      this.showAlert("复制失败，请手动复制", "复制失败", "error");
    }

    document.body.removeChild(tempInput);
  },
};

window.showLogin = () => PortalUI.showLogin();
window.showRegister = () => PortalUI.showRegister();
window.switchTab = (tabName) => PortalUI.switchTab(tabName);
window.navigateToApply = () => PortalUI.navigateToApply();
window.downloadLicense = (id) => PortalUI.downloadLicense(id);
window.viewLicenseDetail = (id) => PortalUI.viewLicenseDetail(id);
window.viewOrderDetail = (id) => PortalUI.viewOrderDetail(id);
window.downloadInvoice = (id) => PortalUI.downloadInvoice(id);
window.copyLicenseKey = (licenseKey) => PortalUI.copyLicenseKey(licenseKey);
window.showAlert = (message, title, type) => PortalUI.showAlert(message, title, type);
window.showDetail = (title, items, actions) => PortalUI.showDetail(title, items, actions);
window.PortalUI = PortalUI;
