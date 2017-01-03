$(function() {
  // Vue debugging
  Vue.config.debug = true;
  Vue.util.warn = function (msg) {console.debug('[Vue warn]: ' + msg)}

  // Register global components
  Vue.component('templated-input', require('./templated-input'));
  Vue.component('message', require('./message'));

  // Vue app
  require('./app');
});
