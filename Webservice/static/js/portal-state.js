window.PortalState = {
  currentUser: null,
  projectsConfig: {},
  currentProjectPricing: null,
  currentTrialAvailability: null,

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
    this.currentTrialAvailability = null;
    this.setToken(null);
  },
};
