const PortalLicenses = {
  currentFilter: "active",

  getTypeMap() {
    return {
      trial: "试用版",
      daily: "天卡",
      weekly: "周卡",
      monthly: "月卡",
      quarterly: "季卡",
      yearly: "年卡",
      perpetual: "永久授权",
    };
  },

  async loadLicenses() {
    const container = document.getElementById("licensesList");
    container.innerHTML = '<div class="loading"><div class="loading-spinner"></div><p>正在加载许可证数据...</p></div>';

    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/license/my-licenses`, {
        headers: { Authorization: `Bearer ${window.PortalState.token}` },
      });

      if (!response.ok) {
        if (response.status === 401 || response.status === 403) {
          await window.PortalAuth.handleAuthFailure(response, "获取许可证列表失败");
          return;
        }
        throw new Error("获取许可证列表失败");
      }

      const licenses = await response.json();
      this.renderLicenses(licenses);
    } catch (error) {
      container.innerHTML = `<div class="alert alert-danger">加载许可证失败: ${error.message}</div>`;
    }
  },

  setFilter(filter) {
    this.currentFilter = ["active", "expired", "revoked"].includes(filter) ? filter : "active";
    document.querySelectorAll(".portal-license-filter").forEach((button) => {
      const isActive = button.dataset.licenseFilter === this.currentFilter;
      button.classList.toggle("btn-primary", isActive);
      button.classList.toggle("btn-secondary", !isActive);
      button.classList.toggle("active", isActive);
    });
    this.loadLicenses();
  },

  renderLicenses(licenses) {
    const container = document.getElementById("licensesList");
    const filteredLicenses = licenses.filter((license) =>
      this.currentFilter === "revoked"
        ? !!license.is_revoked
        : this.currentFilter === "expired"
          ? !license.is_revoked && !!license.is_expired
          : !license.is_revoked && !license.is_expired
    );

    if (licenses.length === 0) {
        container.innerHTML = '<div class="empty-state"><div class="empty-state-icon">LC</div><h3 class="empty-state-title">暂无许可证</h3><p class="empty-state-description">还没有申请任何许可证</p><button class="btn btn-primary" onclick="navigateToApply()">申请许可证</button></div>';
        return;
    }

    if (filteredLicenses.length === 0) {
      container.innerHTML = this.currentFilter === "revoked"
        ? '<div class="empty-state"><div class="empty-state-icon">RV</div><h3 class="empty-state-title">暂无已撤销许可证</h3><p class="empty-state-description">当前没有已撤销的许可证记录</p></div>'
        : this.currentFilter === "expired"
          ? '<div class="empty-state"><div class="empty-state-icon">EX</div><h3 class="empty-state-title">暂无已过期许可证</h3><p class="empty-state-description">当前没有已过期的许可证记录</p></div>'
          : '<div class="empty-state"><div class="empty-state-icon">OK</div><h3 class="empty-state-title">暂无有效许可证</h3><p class="empty-state-description">当前没有可用的有效许可证</p><button class="btn btn-primary" onclick="navigateToApply()">申请许可证</button></div>';
      return;
    }

    const typeMap = this.getTypeMap();
    const html = `
        <div class="table-container portal-license-table-container">
          <table class="table portal-license-table">
            <thead>
              <tr>
                <th>ID</th>
                <th>许可证密钥</th>
                <th>项目ID</th>
                <th>设备ID</th>
                <th>类型</th>
                <th>生成时间</th>
                <th>到期时间</th>
                <th>状态</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody>
              ${filteredLicenses.map((license) => {
                const typeText = typeMap[license.license_type] || license.license_type;
                const expiryTime = license.is_perpetual
                  ? "永久有效"
                  : (license.expiry_time ? new Date(license.expiry_time).toLocaleString("zh-CN") : "-");
                const generateTime = license.generate_time
                  ? new Date(license.generate_time).toLocaleString("zh-CN")
                  : "-";
                const statusText = license.is_revoked ? "已撤销" : (license.is_expired ? "已过期" : "正常");
                const statusClass = license.is_revoked ? "danger" : (license.is_expired ? "warning" : "success");
                const keyText = license.license_key ? `${license.license_key.substring(0, 40)}...` : (license.is_revoked ? "已撤销" : "已过期");
                const actionButtons = (license.is_revoked || license.is_expired)
                  ? '<span class="text-muted">不可操作</span>'
                  : `<button class="btn btn-primary btn-sm" onclick="copyLicenseKey('${license.license_key}')">复制</button><button class="btn btn-secondary btn-sm" onclick="downloadLicense(${license.id})">下载</button>`;
                return `
                  <tr>
                    <td><span class="ellipsis-cell" title="${license.id}">${license.id}</span></td>
                    <td><code class="ellipsis-cell" title="${license.license_key || (license.is_revoked ? '已撤销' : '已过期')}">${keyText}</code></td>
                    <td><span class="ellipsis-cell" title="${license.project_id || "-"}">${license.project_id || "-"}</span></td>
                    <td><span class="ellipsis-cell" title="${license.machine_id || "-"}">${license.machine_id || "-"}</span></td>
                    <td><span class="badge ${license.is_perpetual ? "success" : "info"}">${typeText}</span></td>
                    <td><span class="ellipsis-cell" title="${generateTime}">${generateTime}</span></td>
                    <td><span class="ellipsis-cell" title="${expiryTime}">${expiryTime}</span></td>
                    <td><span class="badge ${statusClass}">${statusText}</span></td>
                    <td>
                      <div class="btn-group" style="gap: 4px;">
                        ${actionButtons}
                      </div>
                    </td>
                  </tr>
                `;
              }).join("")}
            </tbody>
          </table>
        </div>
      `;

    container.innerHTML = html;
  },

  async loadOrders() {
    const container = document.getElementById("ordersList");
    container.innerHTML = '<div class="loading"><div class="loading-spinner"></div><p>正在加载订单数据...</p></div>';

    try {
      const orders = [];
      if (orders.length === 0) {
        container.innerHTML = '<div class="empty-state"><div class="empty-state-icon">OD</div><h3 class="empty-state-title">暂无订单记录</h3><p class="empty-state-description">还没有任何订单记录</p></div>';
        return;
      }

      container.innerHTML = `
        <div class="table-container portal-order-table-container">
          <table class="table portal-order-table">
            <thead>
              <tr>
                <th>订单号</th>
                <th>项目</th>
                <th>类型</th>
                <th>金额</th>
                <th>创建时间</th>
                <th>状态</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody></tbody>
          </table>
        </div>
      `;
    } catch (error) {
      container.innerHTML = `<div class="alert alert-danger">加载订单失败: ${error.message}</div>`;
    }
  },
};

window.setLicenseFilter = (filter) => PortalLicenses.setFilter(filter);
window.PortalLicenses = PortalLicenses;
