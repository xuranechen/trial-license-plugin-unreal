// 应用管理模块
const AdminApplications = {
  applications: [],

  getDefaultPricing() {
    return {
      daily_price: 9.9,
      weekly_price: 49.9,
      monthly_price: 149.9,
      quarterly_price: 399.9,
      yearly_price: 999.9,
      perpetual_price: 2999.0,
    };
  },

  fillPricingFields(pricing = {}) {
    const defaults = this.getDefaultPricing();
    document.getElementById("dailyPrice").value = pricing.daily_price ?? defaults.daily_price;
    document.getElementById("weeklyPrice").value = pricing.weekly_price ?? defaults.weekly_price;
    document.getElementById("monthlyPrice").value = pricing.monthly_price ?? defaults.monthly_price;
    document.getElementById("quarterlyPrice").value = pricing.quarterly_price ?? defaults.quarterly_price;
    document.getElementById("yearlyPrice").value = pricing.yearly_price ?? defaults.yearly_price;
    document.getElementById("perpetualPrice").value = pricing.perpetual_price ?? defaults.perpetual_price;
  },

  formatPricingSummary(app) {
    return [
      `天卡: ¥${Number(app.daily_price ?? 9.9).toFixed(2)}`,
      `周卡: ¥${Number(app.weekly_price ?? 49.9).toFixed(2)}`,
      `月卡: ¥${Number(app.monthly_price ?? 149.9).toFixed(2)}`,
      `季卡: ¥${Number(app.quarterly_price ?? 399.9).toFixed(2)}`,
      `年卡: ¥${Number(app.yearly_price ?? 999.9).toFixed(2)}`,
      `永久: ¥${Number(app.perpetual_price ?? 2999.0).toFixed(2)}`,
    ].join("<br>");
  },

  formatTrialSummary(app) {
    return `${app.trial_duration_hours} 小时 / ${Number(app.trial_request_limit ?? 1)} 次`;
  },
  
  // 加载应用列表（用于选择框）
  async loadList() {
    try {
      this.applications = await AdminAPI.get("/admin/applications");
      this.updateApplicationSelects();
    } catch (error) {
      console.error("加载应用列表失败:", error);
    }
  },
  
  // 更新应用选择框
  updateApplicationSelects() {
    const selects = ["batchApplication", "planApplication"];
    selects.forEach((selectId) => {
      const select = document.getElementById(selectId);
      if (select) {
        select.innerHTML = '<option value="">请选择应用</option>';
        this.applications.forEach((app) => {
          select.innerHTML += `<option value="${app.id}">${app.project_name} (${app.project_id})</option>`;
        });
      }
    });
  },
  
  // 加载应用管理页面
  async loadPage() {
    AdminUI.showLoading("applicationsList");
    
    try {
      const apps = await AdminAPI.get("/admin/applications");
      this.renderApplications(apps);
    } catch (error) {
      AdminUI.showError("applicationsList", "加载应用数据失败: " + error.message);
    }
  },
  
  // 渲染应用列表
  renderApplications(apps) {
    const container = document.getElementById("applicationsList");
    
    if (apps.length === 0) {
      AdminUI.showEmptyState("applicationsList", {
        icon: "APP",
        title: "暂无应用",
        description: "还没有创建任何应用",
        actionText: "创建应用",
        actionHandler: "showCreateApplication()"
      });
      return;
    }
    
    const html = `
      <div class="table-container">
        <table class="table">
          <thead>
            <tr>
              <th>项目ID</th>
              <th>项目名称</th>
              <th>描述</th>
              <th>试用时长</th>
              <th>许可证价格</th>
              <th>创建时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            ${apps.map(app => `
              <tr>
                <td><code>${app.project_id}</code></td>
                <td><strong>${app.project_name}</strong></td>
                <td>${app.description || "无"}</td>
                <td>${this.formatTrialSummary(app)}</td>
                <td>${this.formatPricingSummary(app)}</td>
                <td>${new Date(app.created_at).toLocaleString()}</td>
                <td>
                  <div class="btn-group" style="gap: 4px;">
                    <button class="btn btn-primary btn-sm" onclick="editApplication('${app.id}')">编辑</button>
                    <button class="btn btn-danger btn-sm" onclick="deleteApplication('${app.id}')">删除</button>
                  </div>
                </td>
              </tr>
            `).join("")}
          </tbody>
        </table>
      </div>
    `;
    
    container.innerHTML = html;
  },
  
  // 显示创建应用模态框
  showCreate() {
    document.getElementById("applicationModalTitle").textContent = "创建应用";
    document.getElementById("applicationForm").reset();
    document.getElementById("applicationId").value = "";
    document.getElementById("projectId").disabled = false;
    document.getElementById("trialRequestLimit").value = 1;
    this.fillPricingFields();
    document.getElementById("applicationError").classList.add("hidden");
    document.getElementById("applicationSuccess").classList.add("hidden");
    AdminUI.showModal("applicationModal");
  },
  
  // 编辑应用
  async edit(appId) {
    try {
      const app = this.applications.find(a => a.id === appId);
      if (!app) {
        throw new Error("应用不存在");
      }
      
      document.getElementById("applicationModalTitle").textContent = "编辑应用";
      document.getElementById("applicationId").value = app.id;
      document.getElementById("projectId").value = app.project_id;
      document.getElementById("projectId").disabled = true;
      document.getElementById("projectName").value = app.project_name;
      document.getElementById("projectDescription").value = app.description || "";
      document.getElementById("projectPassphrase").value = app.passphrase || app.project_id || "";
      document.getElementById("trialDurationHours").value = app.trial_duration_hours;
      document.getElementById("trialRequestLimit").value = app.trial_request_limit ?? 1;
      this.fillPricingFields(app);
      
      document.getElementById("applicationError").classList.add("hidden");
      document.getElementById("applicationSuccess").classList.add("hidden");
      AdminUI.showModal("applicationModal");
    } catch (error) {
      AdminUI.showNotification("加载应用数据失败", "error");
    }
  },
  
  // 提交应用表单
  async submitForm(event) {
    event.preventDefault();
    
    const appId = document.getElementById("applicationId").value;
    const projectId = document.getElementById("projectId").value.trim();
    const rawPassphrase = document.getElementById("projectPassphrase").value.trim();
    const normalizedPassphrase =
      !rawPassphrase || ["undefined", "null"].includes(rawPassphrase.toLowerCase())
        ? projectId
        : rawPassphrase;

    const formData = {
      project_id: projectId,
      project_name: document.getElementById("projectName").value,
      description: document.getElementById("projectDescription").value,
      passphrase: normalizedPassphrase,
      trial_duration_hours: parseFloat(document.getElementById("trialDurationHours").value),
      trial_request_limit: parseInt(document.getElementById("trialRequestLimit").value, 10),
      daily_price: parseFloat(document.getElementById("dailyPrice").value),
      weekly_price: parseFloat(document.getElementById("weeklyPrice").value),
      monthly_price: parseFloat(document.getElementById("monthlyPrice").value),
      quarterly_price: parseFloat(document.getElementById("quarterlyPrice").value),
      yearly_price: parseFloat(document.getElementById("yearlyPrice").value),
      perpetual_price: parseFloat(document.getElementById("perpetualPrice").value)
    };
    
    try {
      if (appId) {
        // 更新
        await AdminAPI.put(`/admin/applications/${appId}`, formData);
        this.showSuccess("应用更新成功！");
        AdminUI.showNotification("应用配置已更新", "success");
      } else {
        // 创建
        await AdminAPI.post("/admin/applications", formData);
        this.showSuccess("应用创建成功！");
        AdminUI.showNotification("应用已创建并保存配置", "success");
      }
      
      setTimeout(() => {
        this.closeModal();
        this.loadPage();
        this.loadList();
      }, 1500);
    } catch (error) {
      this.showError(error.message || "操作失败");
    }
  },
  
  // 删除应用
  async delete(appId) {
    AdminUI.showConfirm(
      "确定要删除这个应用吗？此操作不可恢复！",
      "删除应用",
      async () => {
        try {
          await AdminAPI.delete(`/admin/applications/${appId}`);
          AdminUI.showNotification("应用删除成功", "success");
          this.loadPage();
          this.loadList();
        } catch (error) {
          AdminUI.showNotification("删除失败: " + error.message, "error");
        }
      }
    );
  },
  
  // 搜索应用
  search() {
    const query = document.getElementById("applicationSearch").value.toLowerCase();
    // 实现搜索逻辑
    console.log("搜索应用:", query);
  },
  
  // 刷新
  refresh() {
    this.loadPage();
  },
  
  // 关闭模态框
  closeModal() {
    AdminUI.closeModal("applicationModal");
  },
  
  // 显示错误
  showError(message) {
    AdminUI.showNotification(message, "error");
  },
  
  // 显示成功
  showSuccess(message) {
    AdminUI.showNotification(message, "success");
  }
};

// 全局函数
function showCreateApplication() {
  AdminApplications.showCreate();
}

function editApplication(appId) {
  AdminApplications.edit(appId);
}

function deleteApplication(appId) {
  AdminApplications.delete(appId);
}

function submitApplicationForm(event) {
  AdminApplications.submitForm(event);
}

function closeApplicationModal() {
  AdminApplications.closeModal();
}

function searchApplications() {
  AdminApplications.search();
}

function refreshApplications() {
  AdminApplications.refresh();
}
