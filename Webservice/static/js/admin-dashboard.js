// 仪表盘模块
const AdminDashboard = {
  // 加载仪表盘数据
  async load() {
    try {
      const response = await AdminAPI.get("/statistics/overview");
      this.updateStats(response);
      document.getElementById("dashboardLoading").style.display = "none";
    } catch (error) {
      console.error("加载统计数据失败:", error);
      AdminUI.showError("dashboardTab", "加载统计数据失败: " + error.message);
    }
  },
  
  // 更新统计数据
  updateStats(data) {
    // 更新总数
    document.getElementById("totalUsers").textContent = data.users.total;
    document.getElementById("totalLicenses").textContent = data.licenses.total;
    document.getElementById("totalOrders").textContent = data.orders.total;
    document.getElementById("totalRevenue").textContent = "¥" + data.revenue.total.toFixed(2);
    
    // 更新今日数据
    document.getElementById("todayUsers").textContent = data.today?.users || 0;
    document.getElementById("todayOrders").textContent = data.today?.orders || 0;
    document.getElementById("todayRevenue").textContent = data.today?.revenue?.toFixed(2) || "0.00";
    
    // 计算激活率
    const activeRate = data.licenses.total > 0
      ? ((data.licenses.activated / data.licenses.total) * 100).toFixed(1)
      : 0;
    document.getElementById("activeRate").textContent = activeRate;
  }
};
