const AdminSystemConfig = {
  async load() {
    try {
      const config = await AdminAPI.get("/admin/system-config");
      this.fillForm(config);
    } catch (error) {
      AdminUI.showNotification(`加载系统配置失败: ${error.message}`, "error");
    }
  },

  fillForm(config) {
    document.getElementById("smtpEnabled").checked = !!config.smtp_enabled;
    document.getElementById("smtpServer").value = config.smtp_server || "";
    document.getElementById("smtpPort").value = config.smtp_port || 587;
    document.getElementById("smtpUseTls").checked = !!config.smtp_use_tls;
    document.getElementById("smtpUsername").value = config.smtp_username || "";
    document.getElementById("smtpPassword").value = config.smtp_password || "";
    document.getElementById("fromEmail").value = config.from_email || "";
    document.getElementById("fromName").value = config.from_name || "";

    document.getElementById("stripeEnabled").checked = !!config.stripe_enabled;
    document.getElementById("stripeSecretKey").value = config.stripe_secret_key || "";
    document.getElementById("stripePublishableKey").value = config.stripe_publishable_key || "";

    document.getElementById("alipayEnabled").checked = !!config.alipay_enabled;
    document.getElementById("alipayAppId").value = config.alipay_app_id || "";
    document.getElementById("alipayPrivateKey").value = config.alipay_private_key || "";
    document.getElementById("alipayPublicKey").value = config.alipay_public_key || "";

    document.getElementById("wechatEnabled").checked = !!config.wechat_enabled;
    document.getElementById("wechatAppId").value = config.wechat_app_id || "";
    document.getElementById("wechatMchId").value = config.wechat_mch_id || "";
    document.getElementById("wechatApiKey").value = config.wechat_api_key || "";
  },

  getPayload() {
    return {
      smtp_enabled: document.getElementById("smtpEnabled").checked,
      smtp_server: document.getElementById("smtpServer").value.trim(),
      smtp_port: Number(document.getElementById("smtpPort").value || 587),
      smtp_use_tls: document.getElementById("smtpUseTls").checked,
      smtp_username: document.getElementById("smtpUsername").value.trim(),
      smtp_password: document.getElementById("smtpPassword").value,
      from_email: document.getElementById("fromEmail").value.trim(),
      from_name: document.getElementById("fromName").value.trim(),

      stripe_enabled: document.getElementById("stripeEnabled").checked,
      stripe_secret_key: document.getElementById("stripeSecretKey").value.trim(),
      stripe_publishable_key: document.getElementById("stripePublishableKey").value.trim(),

      alipay_enabled: document.getElementById("alipayEnabled").checked,
      alipay_app_id: document.getElementById("alipayAppId").value.trim(),
      alipay_private_key: document.getElementById("alipayPrivateKey").value.trim(),
      alipay_public_key: document.getElementById("alipayPublicKey").value.trim(),

      wechat_enabled: document.getElementById("wechatEnabled").checked,
      wechat_app_id: document.getElementById("wechatAppId").value.trim(),
      wechat_mch_id: document.getElementById("wechatMchId").value.trim(),
      wechat_api_key: document.getElementById("wechatApiKey").value.trim(),
    };
  },

  getSmtpPayload() {
    const payload = this.getPayload();
    return {
      smtp_enabled: payload.smtp_enabled,
      smtp_server: payload.smtp_server,
      smtp_port: payload.smtp_port,
      smtp_use_tls: payload.smtp_use_tls,
      smtp_username: payload.smtp_username,
      smtp_password: payload.smtp_password,
      from_email: payload.from_email,
      from_name: payload.from_name,
    };
  },

  getPaymentPayload() {
    const payload = this.getPayload();
    return {
      stripe_enabled: payload.stripe_enabled,
      stripe_secret_key: payload.stripe_secret_key,
      stripe_publishable_key: payload.stripe_publishable_key,
      alipay_enabled: payload.alipay_enabled,
      alipay_app_id: payload.alipay_app_id,
      alipay_private_key: payload.alipay_private_key,
      alipay_public_key: payload.alipay_public_key,
      wechat_enabled: payload.wechat_enabled,
      wechat_app_id: payload.wechat_app_id,
      wechat_mch_id: payload.wechat_mch_id,
      wechat_api_key: payload.wechat_api_key,
    };
  },

  mergePayload(partial) {
    return {
      ...this.getPayload(),
      ...partial,
    };
  },

  setMessage(prefix, message, isError = false) {
    AdminUI.showNotification(message, isError ? "error" : "success");
  },

  async save(event, section) {
    event.preventDefault();
    try {
      const payload = section === "smtp"
        ? this.mergePayload(this.getSmtpPayload())
        : this.mergePayload(this.getPaymentPayload());
      const config = await AdminAPI.put("/admin/system-config", payload);
      this.fillForm(config);
      this.setMessage(section === "smtp" ? "smtpConfig" : "paymentConfig", section === "smtp" ? "邮件配置已保存" : "支付配置已保存");
      AdminUI.showNotification(section === "smtp" ? "邮件配置已保存" : "支付配置已保存", "success");
    } catch (error) {
      this.setMessage(section === "smtp" ? "smtpConfig" : "paymentConfig", `保存失败: ${error.message}`, true);
    }
  },
};

document.getElementById("smtpConfigForm")?.addEventListener("submit", (event) => AdminSystemConfig.save(event, "smtp"));
document.getElementById("paymentConfigForm")?.addEventListener("submit", (event) => AdminSystemConfig.save(event, "payment"));

function refreshSystemConfig() {
  AdminSystemConfig.load();
}

window.refreshSystemConfig = refreshSystemConfig;
window.AdminSystemConfig = AdminSystemConfig;
