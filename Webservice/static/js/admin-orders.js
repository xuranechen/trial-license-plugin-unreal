// 订单管理模块
const AdminOrders = {
  orders: [],

  // 加载订单列表
  async load() {
    AdminUI.showLoading("ordersList");
    
    try {
      const orders = await AdminAPI.get("/admin/orders");
      this.orders = Array.isArray(orders) ? orders : [];
      this.renderOrders(orders);
    } catch (error) {
      AdminUI.showError("ordersList", "加载订单失败: " + error.message);
    }
  },
  
  // 渲染订单列表
  renderOrders(orders) {
    const container = document.getElementById("ordersList");
    
    if (orders.length === 0) {
      AdminUI.showEmptyState("ordersList", {
        icon: "ORD",
        title: "暂无订单",
        description: "还没有任何订单记录"
      });
      return;
    }
    
    const html = `
      <div class="table-container">
        <table class="table">
          <thead>
            <tr>
              <th>订单号</th>
              <th>用户</th>
              <th>金额</th>
              <th>状态</th>
              <th>创建时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            ${orders.map(order => `
              <tr>
                <td><strong>${order.order_no}</strong></td>
                <td>${order.user_id}</td>
                <td>¥${order.amount.toFixed(2)}</td>
                <td>
                  <span class="badge ${
                    order.status === "paid" ? "success" :
                    order.status === "pending" ? "warning" : "danger"
                  }">
                    ${
                      order.status === "paid" ? "已支付" :
                      order.status === "pending" ? "待支付" : order.status
                    }
                  </span>
                </td>
                <td>${new Date(order.created_at).toLocaleString()}</td>
                <td>
                  <div class="btn-group" style="gap: 4px;">
                    <button class="btn btn-primary btn-sm" onclick="viewOrderDetail('${order.id}')">查看</button>
                    ${order.status === "pending" ? `<button class="btn btn-success btn-sm" onclick="completeOrder('${order.id}')">完成</button>` : ""}
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
  
  // 搜索订单
  search() {
    const query = document.getElementById("orderSearch").value.toLowerCase();
    const filtered = this.orders.filter((order) =>
      !query || [order.order_no, order.user_id, order.id]
        .filter(Boolean)
        .some((value) => String(value).toLowerCase().includes(query))
    );
    this.renderOrders(filtered);
  },
  
  // 刷新
  refresh() {
    this.load();
  }
};

// 全局函数
function searchOrders() {
  AdminOrders.search();
}

function refreshOrders() {
  AdminOrders.refresh();
}

function viewOrderDetail(id) {
  AdminAPI.get(`/admin/orders/${id}`)
    .then((order) => {
      AdminUI.showDetail("订单详情", [
        { label: "订单 ID", value: order.id },
        { label: "订单号", value: order.order_no },
        { label: "用户 ID", value: order.user_id || "-" },
        { label: "应用", value: order.application_name || order.application_id || "-" },
        { label: "金额", value: `¥${Number(order.amount || 0).toFixed(2)}` },
        { label: "状态", value: order.status || "-" },
        { label: "产品", value: order.product_name || "-" },
      ]);
    })
    .catch((error) => {
      AdminUI.showAlert(error.message || "加载订单详情失败", "查看订单", "error");
    });
}

function completeOrder(id) {
  AdminUI.showConfirm("确定要将这个订单标记为已支付吗？", "完成订单", async () => {
    try {
      await AdminAPI.post(`/admin/orders/${id}/complete`, {});
      AdminUI.showNotification("订单已完成", "success");
      await AdminOrders.load();
    } catch (error) {
      AdminUI.showAlert(error.message || "完成订单失败", "完成订单", "error");
    }
  });
}

window.searchOrders = searchOrders;
window.refreshOrders = refreshOrders;
window.viewOrderDetail = viewOrderDetail;
window.completeOrder = completeOrder;
