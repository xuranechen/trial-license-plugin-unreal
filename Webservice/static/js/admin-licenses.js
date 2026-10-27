// 许可证管理模块
const AdminLicenses = {
  licenses: [],
  usersById: {},

  // 加载许可证列表
  async load() {
    AdminUI.showLoading("licensesList");
    
    try {
      // 并行加载许可证和用户映射
      const [licenses, users] = await Promise.all([
        AdminAPI.get("/admin/licenses"),
        AdminAPI.get("/admin/users"),
      ]);
      this.licenses = Array.isArray(licenses) ? licenses : [];
      this.usersById = (Array.isArray(users) ? users : []).reduce((acc, user) => {
        if (user && user.id) {
          acc[user.id] = user;
        }
        return acc;
      }, {});
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
              <th>密钥</th>
              <th>项目ID</th>
              <th>设备</th>
              <th>用户ID</th>
              <th>类型</th>
              <th>水印</th>
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
              const showWatermark = license.show_watermark !== false;
              const watermarkText = showWatermark ? '有' : '免';
              const watermarkClass = showWatermark ? 'warning' : 'success';
              const fullLicenseKey = license.license_key || '-';
              const shortLicenseKey = fullLicenseKey.length > 24
                ? `${fullLicenseKey.slice(0, 10)}...${fullLicenseKey.slice(-8)}`
                : fullLicenseKey;
              const fullMachineId = license.machine_id || '-';
              const shortMachineId = fullMachineId.length > 20
                ? `${fullMachineId.slice(0, 8)}...${fullMachineId.slice(-6)}`
                : fullMachineId;
              const userName = (license.user_id && this.usersById[license.user_id]?.username)
                ? this.usersById[license.user_id].username
                : (license.user_id || '-');
                
              return `
              <tr>
                <td><span class="ellipsis-cell" title="${license.id}">${license.id}</span></td>
                <td><code class="ellipsis-cell" title="${fullLicenseKey}">${shortLicenseKey}</code></td>
                <td><span class="ellipsis-cell" title="${license.project_id || '-'}">${license.project_id || '-'}</span></td>
                <td><span class="ellipsis-cell" title="${fullMachineId}">${shortMachineId}</span></td>
                <td><span class="ellipsis-cell" title="${userName}">${userName}</span></td>
                <td>
                  <span class="badge ${isPerpetual ? "success" : "info"}">
                    ${typeText}
                  </span>
                </td>
                <td>
                  <span class="badge ${watermarkClass}">
                    ${watermarkText}
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
      const watermarkText = detail.show_watermark === false ? "免水印" : "有水印";
      const userName = (detail.user_id && AdminLicenses.usersById[detail.user_id]?.username)
        ? AdminLicenses.usersById[detail.user_id].username
        : (detail.user_id || "-");
      const generateText = detail.generate_time
        ? new Date(detail.generate_time).toLocaleString("zh-CN")
        : "-";
      const expireText = detail.expiry_time
        ? new Date(detail.expiry_time).toLocaleString("zh-CN")
        : (detail.is_perpetual ? "永久" : "-");
      AdminUI.showDetail("许可证详情", [
        { label: "ID", value: detail.id },
        { label: "类型", value: detail.license_type },
        { label: "项目", value: detail.project_id || "-" },
        { label: "设备", value: detail.machine_id || "-" },
        { label: "用户名", value: userName },
        { label: "水印", value: watermarkText },
        { label: "状态", value: statusText },
        { label: "时间", value: `生效 ${generateText} / 到期 ${expireText}` },
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
