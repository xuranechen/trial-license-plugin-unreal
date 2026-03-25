// UI相关功能模块（标签切换、模态框等）
const AdminUI = {
  // 切换标签
  switchTab(tabName) {
    // 更新标签激活状态
    document.querySelectorAll(".tab").forEach((t) => t.classList.remove("active"));
    const tabMap = {
      dashboard: "仪表盘",
      applications: "应用管理",
      licenses: "许可证",
      orders: "订单",
      users: "用户",
      userDetail: "用户",
      systemConfig: "系统配置",
    };
    const activeTab = Array.from(document.querySelectorAll(".admin-sidebar .tab")).find(
      (tab) => tab.textContent.trim() === tabMap[tabName]
    );
    if (activeTab) {
      activeTab.classList.add("active");
    }
    
    // 隐藏所有标签内容
    document.querySelectorAll('[id$="Tab"]').forEach((t) => t.classList.add("hidden"));
    document.getElementById(tabName + "Tab").classList.remove("hidden");
    
    // 根据标签名加载对应数据
    switch (tabName) {
      case "dashboard":
        AdminDashboard.load();
        break;
      case "applications":
        AdminApplications.loadPage();
        break;
      case "licenses":
        AdminLicenses.load();
        break;
      case "orders":
        AdminOrders.load();
        break;
      case "users":
        AdminUsers.load();
        break;
      case "userDetail":
        if (window.AdminUsers && typeof window.AdminUsers.loadDetailPage === "function") {
          window.AdminUsers.loadDetailPage();
        }
        break;
      case "systemConfig":
        AdminSystemConfig.load();
        break;
    }
  },
  
  // 显示模态框
  showModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
      modal.classList.add("show");
      document.body.style.overflow = "hidden";
    }
  },
  
  // 关闭模态框
  closeModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
      modal.classList.remove("show");
      document.body.style.overflow = "auto";
    }
  },
  
  // 显示加载状态
  showLoading(containerId) {
    const container = document.getElementById(containerId);
    if (container) {
      container.innerHTML = `
        <div class="loading">
          <div class="loading-spinner"></div>
          <p>正在加载数据...</p>
        </div>
      `;
    }
  },
  
  // 显示空状态
  showEmptyState(containerId, options = {}) {
    const {
      icon = "--",
      title = "暂无数据",
      description = "还没有任何数据",
      actionText = "",
      actionHandler = null
    } = options;
    
    const container = document.getElementById(containerId);
    if (container) {
      let html = `
        <div class="empty-state">
          <div class="empty-state-icon">${icon}</div>
          <h3 class="empty-state-title">${title}</h3>
          <p class="empty-state-description">${description}</p>
      `;
      
      if (actionText && actionHandler) {
        html += `<button class="btn btn-primary" onclick="${actionHandler}">${actionText}</button>`;
      }
      
      html += `</div>`;
      container.innerHTML = html;
    }
  },
  
  // 显示错误
  showError(containerId, message) {
    const container = document.getElementById(containerId);
    if (container) {
      container.innerHTML = `
        <div class="alert alert-danger">
          ${message}
        </div>
      `;
    }
  },
  
  // 显示通知
  showNotification(message, type = "info") {
    if ((!window.ModalSystem || typeof window.ModalSystem.notification !== "function") && typeof initializeModalSystem === "function") {
      initializeModalSystem();
    }

    if (window.ModalSystem && typeof window.ModalSystem.notification === "function") {
      window.ModalSystem.notification(message, type, 2500);
      return;
    }

    console.log(`[${type.toUpperCase()}] ${message}`);
  },

  showAlert(message, title = "提示", type = "info") {
    if ((!window.ModalSystem || typeof window.ModalSystem.alert !== "function") && typeof initializeModalSystem === "function") {
      initializeModalSystem();
    }

    if (window.ModalSystem && typeof window.ModalSystem.alert === "function") {
      window.ModalSystem.alert(message, title, type);
      return;
    }

    console.log(`[${type.toUpperCase()}] ${title}: ${message}`);
  },

  showDetail(title = "详情", items = []) {
    if ((!window.ModalSystem || typeof window.ModalSystem.detail !== "function") && typeof initializeModalSystem === "function") {
      initializeModalSystem();
    }

    if (window.ModalSystem && typeof window.ModalSystem.detail === "function") {
      window.ModalSystem.detail(title, items);
      return;
    }

    console.log(`[DETAIL] ${title}`, items);
  },
  
  // 显示确认对话框
  showConfirm(message, title, onConfirm) {
    if ((!window.ModalSystem || typeof window.ModalSystem.confirm !== "function") && typeof initializeModalSystem === "function") {
      initializeModalSystem();
    }

    if (window.ModalSystem && typeof window.ModalSystem.confirm === "function") {
      window.ModalSystem.confirm(message, title || "确认操作", onConfirm, null);
      return;
    }

    if (typeof window.showConfirm === "function" && window.showConfirm !== showConfirm) {
      window.showConfirm(message, title || "确认操作", onConfirm, null);
      return;
    }

    if (typeof onConfirm === "function") {
      onConfirm();
    }
  }
};

// 全局函数（向后兼容）
function switchTab(tabName) {
  AdminUI.switchTab(tabName);
}

function showNotification(message, type) {
  AdminUI.showNotification(message, type);
}

function showAlert(message, title, type) {
  AdminUI.showAlert(message, title, type);
}

function showDetail(title, items) {
  AdminUI.showDetail(title, items);
}

function showConfirm(message, title, onConfirm) {
  AdminUI.showConfirm(message, title, onConfirm);
}

// 点击模态框外部关闭
window.onclick = function (event) {
  ["profileModal", "applicationModal", "issueLicenseModal", "benefitModal"].forEach(
    (modalId) => {
      const modal = document.getElementById(modalId);
      if (event.target == modal) {
        modal.classList.remove("show");
        document.body.style.overflow = "auto";
      }
    }
  );
};
