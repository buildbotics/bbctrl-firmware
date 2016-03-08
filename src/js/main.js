$(function() {
  // Vue debugging
  Vue.config.debug = true;
  Vue.util.warn = function (msg) {console.debug('[Vue warn]: ' + msg)}

  // Vue app
  require('./app');
});
