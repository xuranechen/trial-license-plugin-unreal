// 用户管理模块
const AdminUsers = {
  users: [],
  currentDetailUserId: null,
  currentDetail: null,
  currentApplications: [],

  // 加载用户列表
  async load() {
    AdminUI.showLoading("usersList");
    
    try {
      const users = await AdminAPI.get("/admin/users");
      this.users = users;
      this.renderUsers(users);
    } catch (error) {
      AdminUI.showError("usersList", "加载用户失败: " + error.message);
    }
  },
  
  // 渲染用户列表
  renderUsers(users) {
    const container = document.getElementById("usersList");
    
    if (users.length === 0) {
      AdminUI.showEmptyState("usersList", {
        icon: "USR",
        title: "暂无用户",
        description: "还没有注册任何用户"
      });
      return;
    }
    
    const html = `
      <div class="table-container">
        <table class="table">
          <thead>
            <tr>
              <th>用户名</th>
              <th>邮箱</th>
              <th>全名</th>
              <th>公司</th>
              <th>角色</th>
              <th>注册时间</th>
              <th>最后登录</th>
              <th>状态</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            ${users.map(user => `
              <tr>
                <td><strong>${user.username}</strong></td>
                <td>${user.email}</td>
                <td>${user.full_name || "未设置"}</td>
                <td>${user.company || "未设置"}</td>
                <td>
                  ${user.is_admin 
                    ? '<span class="badge badge-warning">管理员</span>' 
                    : '<span class="badge badge-info">用户</span>'}
                </td>
                <td>${new Date(user.created_at).toLocaleDateString()}</td>
                <td>${user.last_login ? new Date(user.last_login).toLocaleString() : "从未登录"}</td>
                <td>
                  <span class="badge ${user.is_active ? "success" : "danger"}">
                    ${user.is_active ? "激活" : "禁用"}
                  </span>
                </td>
                <td>
                  <div class="btn-group" style="gap: 4px;">
                    <button class="btn btn-primary btn-sm" onclick="viewUserDetail('${user.id}')">查看</button>
                    ${user.is_admin
                      ? `<span class="text-muted"></span>`
                      : (user.is_active
                        ? `<button class="btn btn-warning btn-sm" onclick="toggleUserStatus('${user.id}', false)">禁用</button>`
                        : `<button class="btn btn-success btn-sm" onclick="toggleUserStatus('${user.id}', true)">启用</button>`)}
                    ${!user.is_admin 
                      ? `<button class="btn btn-danger btn-sm" onclick="deleteUser('${user.id}')">删除</button>`
                      : ""}
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
  
  // 搜索用户
  search() {
    const query = document.getElementById("userSearch").value.toLowerCase();
    const filtered = this.users.filter((user) =>
      !query || [user.username, user.email, user.company, user.full_name]
        .filter(Boolean)
        .some((value) => String(value).toLowerCase().includes(query))
    );
    this.renderUsers(filtered);
  },
  
  // 刷新
  refresh() {
    this.load();
  },

  async openDetail(userId) {
    this.currentDetailUserId = userId;
    window.switchTab("userDetail");
  },

  async loadDetailPage() {
    if (!this.currentDetailUserId) return;
    const container = document.getElementById("userDetailContent");
    container.innerHTML = '<div class="loading"><div class="loading-spinner"></div><p>正在加载用户详情...</p></div>';
    try {
      const [detail, apps] = await Promise.all([
        AdminAPI.get(`/admin/users/${this.currentDetailUserId}/detail`),
        AdminAPI.get("/admin/applications"),
      ]);
      this.currentDetail = detail;
      this.currentApplications = apps.filter((app) => app.is_active);
      this.renderDetail(detail, apps);
    } catch (error) {
      container.innerHTML = `<div class="alert alert-danger">加载用户详情失败: ${error.message}</div>`;
    }
  },

  renderDetail(detail, apps) {
    const appOptions = apps.filter((app) => app.is_active).map((app) => `<option value="${app.project_id}">${app.project_name} (${app.project_id})</option>`).join("");
    const appRows = detail.applications.length ? detail.applications.map((app) => `
      <tr>
        <td>${app.project_name}</td>
        <td>${app.license_count}</td>
        <td>${app.device_count}</td>
        <td>${app.extra_trial_requests} 次 / ${app.extra_trial_duration_hours} 小时</td>
      </tr>
    `).join("") : '<tr><td colspan="4">暂无应用数据</td></tr>';
    const deviceRows = detail.devices.length ? detail.devices.map((device) => `
      <tr>
        <td>${device.machine_id}</td>
        <td>${device.project_count}</td>
        <td>${device.license_count}</td>
        <td>${device.latest_license_time ? new Date(device.latest_license_time).toLocaleString("zh-CN") : "-"}</td>
      </tr>
    `).join("") : '<tr><td colspan="4">暂无设备数据</td></tr>';
    const licenseRows = detail.recent_licenses.length ? detail.recent_licenses.map((license) => `
      <tr>
        <td>${license.project_id}</td>
        <td>${license.machine_id}</td>
        <td>${license.license_type}</td>
        <td>${license.is_revoked ? "已撤销" : (license.is_expired ? "已过期" : "正常")}</td>
      </tr>
    `).join("") : '<tr><td colspan="4">暂无许可证数据</td></tr>';
    const userStatus = detail.is_active ? "正常" : "已封禁";
    const userRole = detail.is_admin ? "管理员" : "普通用户";

    document.getElementById("userDetailContent").innerHTML = `
      <div class="user-detail-page">
        <div class="user-summary-row">
          <div class="card user-overview-card user-overview-card-accent">
            <div class="user-overview-head">
              <div>
                <div class="user-overview-eyebrow">用户档案</div>
                <h3 class="card-title">${detail.username}</h3>
                <p class="card-subtitle">${detail.email}</p>
              </div>
              <div class="user-overview-badges">
                <span class="badge ${detail.is_active ? "success" : "danger"}">${userStatus}</span>
                <span class="badge ${detail.is_admin ? "warning" : "info"}">${userRole}</span>
              </div>
            </div>
            <div class="user-meta-grid">
              <div class="detail-row"><div class="detail-label">姓名</div><div class="detail-value">${detail.full_name || "未设置"}</div></div>
              <div class="detail-row"><div class="detail-label">公司</div><div class="detail-value">${detail.company || "未设置"}</div></div>
              <div class="detail-row"><div class="detail-label">注册时间</div><div class="detail-value">${detail.created_at ? new Date(detail.created_at).toLocaleString("zh-CN") : "-"}</div></div>
              <div class="detail-row"><div class="detail-label">最后登录</div><div class="detail-value">${detail.last_login ? new Date(detail.last_login).toLocaleString("zh-CN") : "从未登录"}</div></div>
            </div>
          </div>
          <div class="user-stat-grid user-stat-grid-wide">
            <div class="user-stat-card stat-blue"><div class="user-stat-label">许可证总数</div><div class="user-stat-value">${detail.total_licenses}</div></div>
            <div class="user-stat-card stat-green"><div class="user-stat-label">有效许可证</div><div class="user-stat-value">${detail.active_licenses}</div></div>
            <div class="user-stat-card stat-cyan"><div class="user-stat-label">设备数</div><div class="user-stat-value">${detail.total_devices}</div></div>
            <div class="user-stat-card stat-indigo"><div class="user-stat-label">应用数</div><div class="user-stat-value">${detail.total_applications}</div></div>
            <div class="user-stat-card stat-rose"><div class="user-stat-label">已撤销</div><div class="user-stat-value">${detail.revoked_licenses}</div></div>
            <div class="user-stat-card stat-amber"><div class="user-stat-label">已过期</div><div class="user-stat-value">${detail.expired_licenses}</div></div>
          </div>
        </div>

        <div class="card section-card-apps"><div class="card-header compact-card-header"><div><h3 class="card-title">应用统计</h3><p class="card-subtitle">用户在每个应用下的许可证和试用权益情况</p></div><div class="btn-group"><button class="btn btn-primary btn-sm" type="button" onclick="openIssueLicenseModal()">下发许可证</button><button class="btn btn-secondary btn-sm" type="button" onclick="openBenefitModal()">试用权益调整</button></div></div><table class="mini-table"><thead><tr><th>应用</th><th>许可证</th><th>设备</th><th>试用权益</th></tr></thead><tbody>${appRows}</tbody></table></div>
        <div class="card section-card-devices"><div class="card-header compact-card-header"><div><h3 class="card-title">设备统计</h3><p class="card-subtitle">该用户使用过的设备和覆盖应用情况</p></div></div><table class="mini-table"><thead><tr><th>设备ID</th><th>应用数</th><th>许可证</th><th>最近使用</th></tr></thead><tbody>${deviceRows}</tbody></table></div>
        <div class="card section-card-licenses"><div class="card-header compact-card-header"><div><h3 class="card-title">最近许可证</h3><p class="card-subtitle">最近生成或更新的许可证记录</p></div></div><table class="mini-table"><thead><tr><th>应用</th><th>设备</th><th>类型</th><th>状态</th></tr></thead><tbody>${licenseRows}</tbody></table></div>
      </div>`;
  },

  populateIssueModal() {
    const projectSelect = document.getElementById("issueProjectId");
    const deviceSelect = document.getElementById("issueMachineSelect");
    if (!projectSelect || !deviceSelect) return;
    projectSelect.innerHTML = `<option value="">请选择应用</option>${this.currentApplications.map((app) => `<option value="${app.project_id}">${app.project_name} (${app.project_id})</option>`).join("")}`;
    deviceSelect.innerHTML = `<option value="">从现有设备选择（可选）</option>${(this.currentDetail?.devices || []).map((device) => `<option value="${device.machine_id}">${device.machine_id}</option>`).join("")}`;
    document.getElementById("issueMachineId").value = "";
    document.getElementById("issueLicenseType").value = "trial";
    document.getElementById("issueDurationHours").value = "";
    this.toggleIssueDurationField();
    document.getElementById("issueLicenseError").classList.add("hidden");
  },

  toggleIssueDurationField() {
    const type = document.getElementById("issueLicenseType")?.value;
    const group = document.getElementById("issueDurationGroup");
    if (!group) return;
    group.style.display = type === "trial" ? "block" : "none";
  },

  populateBenefitModal() {
    const select = document.getElementById("benefitProjectId");
    if (!select) return;
    select.innerHTML = `<option value="">请选择应用</option>${this.currentApplications.map((app) => `<option value="${app.project_id}">${app.project_name} (${app.project_id})</option>`).join("")}`;
    document.getElementById("benefitTrialRequests").value = 0;
    document.getElementById("benefitTrialHours").value = 0;
    document.getElementById("benefitError").classList.add("hidden");
  },

  openIssueModal() {
    this.populateIssueModal();
    AdminUI.showModal("issueLicenseModal");
  },

  openBenefitModal() {
    this.populateBenefitModal();
    AdminUI.showModal("benefitModal");
  },

  async submitIssueLicense(event) {
    event.preventDefault();
    try {
      await AdminAPI.post(`/admin/users/${this.currentDetailUserId}/issue-license`, {
        project_id: document.getElementById("issueProjectId").value,
        machine_id: document.getElementById("issueMachineId").value.trim(),
        license_type: document.getElementById("issueLicenseType").value,
        duration_hours: document.getElementById("issueLicenseType").value === "trial"
          ? Number(document.getElementById("issueDurationHours").value || 0) || null
          : null,
      });
      AdminUI.showNotification("许可证已下发到用户账户", "success");
      AdminUI.closeModal("issueLicenseModal");
      this.loadDetailPage();
    } catch (error) {
      AdminUI.showNotification(error.message || "下发许可证失败", "error");
    }
  },

  async submitBenefit(event) {
    event.preventDefault();
    try {
      await AdminAPI.post(`/admin/users/${this.currentDetailUserId}/trial-benefits`, {
        project_id: document.getElementById("benefitProjectId").value,
        extra_trial_requests: Number(document.getElementById("benefitTrialRequests").value || 0),
        extra_trial_duration_hours: Number(document.getElementById("benefitTrialHours").value || 0),
      });
      AdminUI.showNotification("试用权益已更新", "success");
      AdminUI.closeModal("benefitModal");
      this.loadDetailPage();
    } catch (error) {
      AdminUI.showNotification(error.message || "更新试用权益失败", "error");
    }
  }
};

// 全局函数
function searchUsers() {
  AdminUsers.search();
}

function refreshUsers() {
  AdminUsers.refresh();
}

function viewUserDetail(id) {
  AdminUsers.openDetail(id);
}

function toggleUserStatus(id, active) {
  const action = active ? "启用" : "禁用";
  AdminUI.showConfirm(
    `确定要${action}这个用户吗？`,
    `${action}用户`,
    async () => {
      try {
        await AdminAPI.request(`/admin/users/${id}/status`, {
          method: "PATCH",
          body: JSON.stringify({ is_active: active }),
        });
        AdminUI.showNotification(`用户${action}成功`, "success");
        await AdminUsers.load();
      } catch (error) {
        AdminUI.showAlert(error.message || `用户${action}失败`, `${action}用户`, "error");
      }
    }
  );
}

function deleteUser(id) {
  AdminUI.showConfirm(
    "确定要删除这个用户吗？此操作不可恢复！",
    "删除用户",
    async () => {
      try {
        await AdminAPI.delete(`/admin/users/${id}`);
        AdminUI.showNotification("用户删除成功", "success");
        await AdminUsers.load();
      } catch (error) {
        AdminUI.showAlert(error.message || "删除用户失败", "删除用户", "error");
      }
    }
  );
}

function backToUsers() {
  AdminUsers.currentDetailUserId = null;
  window.switchTab("users");
}

function refreshUserDetail() {
  AdminUsers.loadDetailPage();
}

function openIssueLicenseModal() {
  AdminUsers.openIssueModal();
}

function closeIssueLicenseModal() {
  AdminUI.closeModal("issueLicenseModal");
}

function openBenefitModal() {
  AdminUsers.openBenefitModal();
}

function closeBenefitModal() {
  AdminUI.closeModal("benefitModal");
}

document.getElementById("issueMachineSelect")?.addEventListener("change", (event) => {
  const target = event.target;
  document.getElementById("issueMachineId").value = target.value || "";
});

document.getElementById("issueLicenseType")?.addEventListener("change", () => {
  AdminUsers.toggleIssueDurationField();
});

document.getElementById("issueLicenseForm")?.addEventListener("submit", (event) => AdminUsers.submitIssueLicense(event));
document.getElementById("benefitForm")?.addEventListener("submit", (event) => AdminUsers.submitBenefit(event));

window.searchUsers = searchUsers;
window.refreshUsers = refreshUsers;
window.viewUserDetail = viewUserDetail;
window.toggleUserStatus = toggleUserStatus;
window.deleteUser = deleteUser;
window.backToUsers = backToUsers;
window.refreshUserDetail = refreshUserDetail;
window.openIssueLicenseModal = openIssueLicenseModal;
window.closeIssueLicenseModal = closeIssueLicenseModal;
window.openBenefitModal = openBenefitModal;
window.closeBenefitModal = closeBenefitModal;
window.AdminUsers = AdminUsers;
