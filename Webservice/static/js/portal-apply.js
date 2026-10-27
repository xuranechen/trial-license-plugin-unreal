const PortalApply = {
  getLicenseConfig() {
    const hasProject = Boolean(window.PortalState.currentProjectPricing);
    const projectPricing = window.PortalState.currentProjectPricing || {};
    const trialHours = projectPricing.trial_duration_hours || 72;
    const trialLimit = Number(projectPricing.trial_request_limit ?? 1);
    const dailyPrice = Number(projectPricing.daily_price ?? 9.9);
    const weeklyPrice = Number(projectPricing.weekly_price ?? 49.9);
    const monthlyPrice = Number(projectPricing.monthly_price ?? 149.9);
    const quarterlyPrice = Number(projectPricing.quarterly_price ?? 399.9);
    const yearlyPrice = Number(projectPricing.yearly_price ?? 999.9);
    const perpetualPrice = Number(projectPricing.perpetual_price ?? 2999.0);

    return {
      trial: { name: "试用版", price: 0, duration: `${trialHours}小时`, inlineText: hasProject ? `免费 · ${trialHours}小时 · 最多${trialLimit}次` : "免费" },
      daily: { price: dailyPrice, name: "天卡", duration: "1天", inlineText: hasProject ? `¥${dailyPrice.toFixed(2)} · 1天` : "1天" },
      weekly: { price: weeklyPrice, name: "周卡", duration: "7天", inlineText: hasProject ? `¥${weeklyPrice.toFixed(2)} · 7天` : "7天" },
      monthly: { price: monthlyPrice, name: "月卡", duration: "30天", inlineText: hasProject ? `¥${monthlyPrice.toFixed(2)} · 30天` : "30天" },
      quarterly: { price: quarterlyPrice, name: "季卡", duration: "90天", inlineText: hasProject ? `¥${quarterlyPrice.toFixed(2)} · 90天` : "90天" },
      yearly: { price: yearlyPrice, name: "年卡", duration: "365天", inlineText: hasProject ? `¥${yearlyPrice.toFixed(2)} · 365天` : "365天" },
      perpetual: { price: perpetualPrice, name: "永久授权", duration: "永久", inlineText: hasProject ? `¥${perpetualPrice.toFixed(2)} · 永久` : "永久" },
    };
  },

  renderLicenseTypeDetails() {
    const config = this.getLicenseConfig();
    document.querySelectorAll(".radio-option[data-license-type]").forEach((option) => {
      const type = option.dataset.licenseType;
      const detail = option.querySelector(".radio-label small");
      if (!type || !detail || !config[type]) return;
      detail.textContent = config[type].inlineText;
    });
  },

  renderTrialAvailability(result = null) {
    const trialOption = document.querySelector('.radio-option[data-license-type="trial"] .radio-label small');
    if (!trialOption) return;

    if (!result) {
      const config = this.getLicenseConfig();
      trialOption.textContent = config.trial.inlineText;
      this.updateNoWatermarkOption(null);
      return;
    }

    const durationText = `${Number(result.effective_trial_duration_hours || 0)}小时`;
    const extraText = Number(result.extra_trial_duration_hours || 0) > 0
      ? ` · 单次+${Number(result.extra_trial_duration_hours)}小时`
      : "";
    trialOption.textContent = result.allowed
      ? `免费 · ${durationText} · 剩余${result.remaining_trial_requests}/${result.trial_request_limit}次${extraText}`
      : `免费 · ${durationText} · 已用完(0/${result.trial_request_limit})${extraText}`;

    this.updateNoWatermarkOption(result);
  },

  updateNoWatermarkOption(result = null) {
    const group = document.getElementById("applyNoWatermarkGroup");
    const checkbox = document.getElementById("applyRequestNoWatermark");
    const hint = document.getElementById("applyNoWatermarkHint");
    const selectedType = document.querySelector('input[name="licenseType"]:checked')?.value;
    if (!group || !checkbox || !hint) return;

    if (!result || !result.allow_no_watermark_trial || selectedType !== "trial") {
      group.classList.add("hidden");
      checkbox.checked = false;
      hint.textContent = "免水印试用权益次数：0/0";
      return;
    }

    group.classList.remove("hidden");
    const remaining = Number(result.remaining_no_watermark_trial_requests || 0);
    const limit = Number(result.no_watermark_trial_request_limit || 0);
    hint.textContent = `免水印试用权益次数：${remaining}/${limit}`;
    checkbox.disabled = remaining <= 0;
    if (remaining <= 0) {
      checkbox.checked = false;
    }
  },

  async checkTrialAvailability() {
    const projectId = document.getElementById("applyProjectName")?.value;
    const encryptedToken = document.getElementById("applyEncryptedToken")?.value.trim();
    const trialOption = document.querySelector('.radio-option[data-license-type="trial"] .radio-label small');
    if (!projectId || !encryptedToken || !trialOption) {
      window.PortalState.currentTrialAvailability = null;
      this.renderLicenseTypeDetails();
      return;
    }

    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/license/trial-availability`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${window.PortalState.token}`,
        },
        body: JSON.stringify({ project_id: projectId, encrypted_token: encryptedToken, license_type: "trial" }),
      });

      if (!response.ok) {
        if (response.status === 401 || response.status === 403) {
          await window.PortalAuth.handleAuthFailure(response, "无法检查试用次数");
          return;
        }
        window.PortalState.currentTrialAvailability = null;
        this.renderLicenseTypeDetails();
        return;
      }

      const result = await response.json();
      window.PortalState.currentTrialAvailability = result;
      this.renderTrialAvailability(result);
    } catch (error) {
      window.PortalState.currentTrialAvailability = null;
      this.renderLicenseTypeDetails();
    }
  },

  getContainer() {
    return document.getElementById("applyPanel");
  },

  async loadProjects() {
    const projectSelect = document.getElementById("applyProjectName");
    if (!projectSelect) return;

    window.PortalState.currentProjectPricing = null;
    window.PortalState.currentTrialAvailability = null;
    this.renderLicenseTypeDetails();
    projectSelect.innerHTML = '<option value="">正在加载项目列表...</option>';
    projectSelect.disabled = true;

    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/admin/applications`, {
        headers: { Authorization: `Bearer ${window.PortalState.token}` },
      });

      if (!response.ok) {
        if (response.status === 401 || response.status === 403) {
          await window.PortalAuth.handleAuthFailure(response, "加载项目失败");
          return;
        }
        projectSelect.innerHTML = '<option value="">加载失败，请重试</option>';
        projectSelect.disabled = true;
        return;
      }

      const projects = await response.json();
      window.PortalState.projectsConfig = {};
      const activeProjects = projects.filter((project) => project.is_active);

      projectSelect.innerHTML = "";
      const defaultOption = document.createElement("option");
      defaultOption.value = "";
      defaultOption.textContent = "请选择项目";
      projectSelect.appendChild(defaultOption);

      if (activeProjects.length === 0) {
        const noProjectOption = document.createElement("option");
        noProjectOption.value = "";
        noProjectOption.textContent = "暂无可用项目";
        projectSelect.innerHTML = "";
        projectSelect.appendChild(noProjectOption);
        projectSelect.disabled = true;
        return;
      }

      activeProjects.forEach((project) => {
        const option = document.createElement("option");
        option.value = project.project_id;
        option.textContent = project.project_name;
        projectSelect.appendChild(option);
        window.PortalState.projectsConfig[project.project_id] = project;
      });

      projectSelect.selectedIndex = 0;
      projectSelect.disabled = false;
      window.PortalState.currentProjectPricing = null;
      window.PortalState.currentTrialAvailability = null;
      this.renderLicenseTypeDetails();
    } catch (error) {
      window.PortalState.currentProjectPricing = null;
      window.PortalState.currentTrialAvailability = null;
      this.renderLicenseTypeDetails();
      projectSelect.innerHTML = '<option value="">加载失败，请重试</option>';
      projectSelect.disabled = true;
    }
  },

  loadProjectPricing() {
    const projectId = document.getElementById("applyProjectName").value;
    if (projectId && window.PortalState.projectsConfig[projectId]) {
      window.PortalState.currentProjectPricing = window.PortalState.projectsConfig[projectId];
      this.updatePriceInfo();
      this.checkTrialAvailability();
    } else {
      window.PortalState.currentProjectPricing = null;
      window.PortalState.currentTrialAvailability = null;
      this.renderLicenseTypeDetails();
    }
  },

  updatePriceInfo() {
    this.renderLicenseTypeDetails();
    this.renderTrialAvailability(window.PortalState.currentTrialAvailability || null);
    this.updateNoWatermarkOption(window.PortalState.currentTrialAvailability || null);
  },

  async submit() {
    const projectId = document.getElementById("applyProjectName").value;
    const encryptedToken = document.getElementById("applyEncryptedToken").value.trim();
    const licenseTypeRadio = document.querySelector('input[name="licenseType"]:checked');
    const licenseType = licenseTypeRadio ? licenseTypeRadio.value : "";
    const email = document.getElementById("applyEmail").value.trim();
    const requestNoWatermark = document.getElementById("applyRequestNoWatermark")?.checked || false;

    if (!projectId || !encryptedToken || !licenseType) {
      this.showApplyError("请填写所有必填字段（项目、加密口令、许可证类型）");
      return;
    }

    const formData = {
      project_id: projectId,
      encrypted_token: encryptedToken,
      license_type: licenseType,
      request_no_watermark: licenseType === "trial" ? requestNoWatermark : false,
    };
    if (email) {
      formData.email = email;
    }

    try {
      const response = await fetch(`${window.PortalConfig.API_BASE}/license/request`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${window.PortalState.token}`,
        },
        body: JSON.stringify(formData),
      });

      if (!response.ok) {
        if (response.status === 401 || response.status === 403) {
          await window.PortalAuth.handleAuthFailure(response, "申请失败");
          return;
        }
        const error = await response.json();
        this.showApplyError(error.detail || "申请失败");
        window.PortalUI.showAlert(error.detail || "申请失败", "试用申请提示", "warning");
        this.checkTrialAvailability();
        return;
      }

      const result = await response.json();
      if (!result.success) {
        this.showPaymentDialog(result);
        return;
      }

      const emailMsg = email || window.PortalState.currentUser?.email || "您的邮箱";
      this.resetApplyForm();
      document.getElementById("applySuccess")?.classList.add("hidden");
      document.getElementById("applyMessage")?.classList.add("hidden");
      window.PortalUI.switchTab("licenses");
      this.showLicenseSuccessModal(result, emailMsg);
      this.checkTrialAvailability();
    } catch (error) {
      this.showApplyError("申请失败：" + error.message);
    }
  },

  showLicenseSuccessModal(result, emailMsg) {
    const remainingText = result.remaining_trial_requests == null || result.trial_request_limit == null
      ? "-"
      : `${result.remaining_trial_requests}/${result.trial_request_limit}`;
    const noWatermarkRemainingText = result.remaining_no_watermark_trial_requests == null || result.no_watermark_trial_request_limit == null
      ? "-"
      : `${result.remaining_no_watermark_trial_requests}/${result.no_watermark_trial_request_limit}`;
    const maskedLicenseKey = result.license_key
      ? `${result.license_key.slice(0, 12)} ... ${result.license_key.slice(-12)}`
      : "-";
    window.PortalUI.showDetail("许可证申请成功", [
      { label: "许可证类型", value: result.license_type || "trial" },
      { label: "许可证密钥", value: maskedLicenseKey },
      { label: "试用时长", value: result.duration_hours ? `${result.duration_hours} 小时` : "-" },
      { label: "剩余试用次数", value: remainingText },
      { label: "免水印", value: result.show_watermark ? "否（有水印）" : "是（免水印）" },
      { label: "剩余免水印次数", value: noWatermarkRemainingText },
      { label: "发送邮箱", value: emailMsg },
    ], [
      {
        label: "复制密钥",
        className: "btn btn-secondary",
        closeOnClick: false,
        onClick: () => window.PortalUI.copyLicenseKey(result.license_key || ""),
      },
      {
        label: "关闭",
        className: "btn btn-primary",
      },
    ]);
  },

  showPaymentDialog(orderInfo) {
    window.PortalUI.showAlert(`订单号：${orderInfo.order_no}\n金额：¥${orderInfo.amount.toFixed(2)}`, "订单支付", "info");
  },

  resetApplyForm() {
    document.getElementById("applyForm").reset();
    window.PortalState.currentProjectPricing = null;
    window.PortalState.currentTrialAvailability = null;
    this.renderLicenseTypeDetails();
    this.updateNoWatermarkOption(null);
    const container = this.getContainer();
    if (container) {
      container.scrollTop = 0;
    }
  },

  showApplyError(message) {
    window.PortalUI.showNotification(message, "error", 5000);
  },

  showApplySuccess(message) {
    window.PortalUI.showNotification(message, "success", 3000);
  },
};

window.loadProjectPricing = () => PortalApply.loadProjectPricing();
window.updatePriceInfo = () => PortalApply.updatePriceInfo();
window.resetApplyForm = () => PortalApply.resetApplyForm();
window.checkTrialAvailability = () => PortalApply.checkTrialAvailability();
window.PortalApply = PortalApply;
