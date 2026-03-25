window.PortalState = {
  currentUser: null,
  projectsConfig: {},
  currentProjectPricing: null,

  get token() {
    return window.PortalConfig.getToken();
  },

  setToken(token) {
    window.PortalConfig.setToken(token);
  },

  clear() {
    this.currentUser = null;
    this.projectsConfig = {};
    this.currentProjectPricing = null;
    this.setToken(null);
  },
};
