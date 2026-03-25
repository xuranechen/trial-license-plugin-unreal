// 许可证管理模块
const AdminLicenses = {
  licenses: [],

  // 加载许可证列表
  async load() {
    AdminUI.showLoading("licensesList");
    
    try {
      // 从API获取许可证列表
      const licenses = await AdminAPI.get("/admin/licenses");
      this.licenses = Array.isArray(licenses) ? licenses : [];
      this.renderLicenses(licenses);
    } catch (error) {
      AdminUI.showError("licensesList", "加载许可证失败: " + error.message);
    }
  },
  
  // 渲染许可证列表
  renderLicenses(licenses) {
    const container = document.getElementById("licensesList");
    
    if (licenses.length === 0) {
      AdminUI.showEmptyState("licensesList", {
        icon: "LIC",
        title: "暂无许可证",
        description: "还没有生成任何许可证"
      });
      return;
    }
    
    // 许可证类型映射
    const typeMap = {
      'trial': '试用版',
      'daily': '天卡',
      'weekly': '周卡',
      'monthly': '月卡',
      'quarterly': '季卡',
      'yearly': '年卡',
      'perpetual': '永久授权'
    };
    
    const html = `
      <div class="table-container license-table-container">
        <table class="table license-table">
          <thead>
            <tr>
              <th>ID</th>
              <th>许可证密钥</th>
              <th>项目ID</th>
              <th>设备ID</th>
              <th>用户ID</th>
              <th>类型</th>
              <th>生成时间</th>
              <th>到期时间</th>
              <th>状态</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            ${licenses.map(license => {
              const typeText = typeMap[license.license_type] || license.license_type;
              const isPerpetual = license.is_perpetual || license.license_type === 'perpetual';
              const expiryTime = isPerpetual ? '永久有效' : (license.expiry_time ? new Date(license.expiry_time).toLocaleString('zh-CN') : '-');
              const statusText = license.is_revoked ? '已撤销' : (license.is_expired ? '已过期' : '正常');
              const statusClass = license.is_revoked ? 'danger' : (license.is_expired ? 'warning' : 'success');
               
              return `
              <tr>
                <td><span class="ellipsis-cell" title="${license.id}">${license.id}</span></td>
                <td><code class="ellipsis-cell" title="${license.license_key}">${license.license_key.substring(0, 40)}...</code></td>
                <td><span class="ellipsis-cell" title="${license.project_id || '-'}">${license.project_id || '-'}</span></td>
                <td><span class="ellipsis-cell" title="${license.machine_id || '-'}">${license.machine_id || '-'}</span></td>
                <td><span class="ellipsis-cell" title="${license.user_id || '-'}">${license.user_id || '-'}</span></td>
                <td>
                  <span class="badge ${isPerpetual ? "success" : "info"}">
                    ${typeText}
                  </span>
                </td>
                <td><span class="ellipsis-cell" title="${license.generate_time ? new Date(license.generate_time).toLocaleString('zh-CN') : '-'}">${license.generate_time ? new Date(license.generate_time).toLocaleString('zh-CN') : '-'}</span></td>
                <td><span class="ellipsis-cell" title="${expiryTime}">${expiryTime}</span></td>
                <td>
                  <span class="badge ${statusClass}">
                    ${statusText}
                  </span>
                </td>
                <td>
                  <div class="btn-group" style="gap: 4px;">
                    <button class="btn btn-primary btn-sm" onclick="viewLicenseDetail('${license.id}')">查看</button>
                    ${!license.is_revoked && !license.is_expired ? `<button class="btn btn-danger btn-sm" onclick="revokeLicense('${license.id}')">撤销</button>` : ""}
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
  
  // 搜索许可证
  search() {
    const query = document.getElementById("licenseSearch").value.toLowerCase();
    const type = document.getElementById("licenseTypeFilter").value;
    const status = document.getElementById("licenseStatusFilter").value;

    const filtered = this.licenses.filter((license) => {
      const matchesQuery = !query || [license.license_key, license.project_id, license.machine_id, license.user_id]
        .filter(Boolean)
        .some((value) => String(value).toLowerCase().includes(query));

      const matchesType = !type || license.license_type === type;
      const currentStatus = license.is_revoked ? "revoked" : (license.is_expired ? "expired" : "active");
      const matchesStatus = !status || currentStatus === status;

      return matchesQuery && matchesType && matchesStatus;
    });

    this.renderLicenses(filtered);
  },
  
  // 清除筛选
  clearFilters() {
    document.getElementById("licenseSearch").value = "";
    document.getElementById("licenseTypeFilter").value = "";
    document.getElementById("licenseStatusFilter").value = "";
    this.load();
  },
  
  // 刷新
  refresh() {
    this.load();
  }
};

function searchLicenses() {
  AdminLicenses.search();
}

function clearLicenseFilters() {
  AdminLicenses.clearFilters();
}

function refreshLicenses() {
  AdminLicenses.refresh();
}

function viewLicenseDetail(id) {
  const license = AdminLicenses.licenses.find((item) => String(item.id) === String(id));
  if (!license) {
    AdminUI.showAlert("未找到对应的许可证记录", "查看许可证", "warning");
    return;
  }

  AdminAPI.get(`/admin/licenses/${id}`)
    .then((detail) => {
      const statusText = detail.is_revoked ? "已撤销" : (detail.is_expired ? "已过期" : "正常");
      AdminUI.showDetail("许可证详情", [
        { label: "许可证 ID", value: detail.id },
        { label: "许可证类型", value: detail.license_type },
        { label: "项目 ID", value: detail.project_id || "-" },
        { label: "设备 ID", value: detail.machine_id || "-" },
        { label: "用户 ID", value: detail.user_id || "-" },
        { label: "状态", value: statusText },
        { label: "生成时间", value: detail.generate_time ? new Date(detail.generate_time).toLocaleString("zh-CN") : "-" },
        { label: "到期时间", value: detail.expiry_time ? new Date(detail.expiry_time).toLocaleString("zh-CN") : (detail.is_perpetual ? "永久" : "-") },
      ]);
    })
    .catch((error) => {
      AdminUI.showAlert(error.message || "加载许可证详情失败", "查看许可证", "error");
    });
}

function revokeLicense(id) {
  AdminUI.showConfirm(
    "确定要撤销这个许可证吗？此操作不可恢复！",
    "撤销许可证",
    async () => {
      try {
        await AdminAPI.post(`/admin/licenses/${id}/revoke`, {});
        AdminUI.showNotification("许可证撤销成功", "success");
        await AdminLicenses.load();
      } catch (error) {
        AdminUI.showAlert(error.message || "撤销许可证失败", "撤销许可证", "error");
      }
    }
  );
}

window.searchLicenses = searchLicenses;
window.clearLicenseFilters = clearLicenseFilters;
window.refreshLicenses = refreshLicenses;
window.viewLicenseDetail = viewLicenseDetail;
window.revokeLicense = revokeLicense;
