$(function() {
  // Vue debugging
  Vue.config.debug = true;
  Vue.util.warn = function (msg) {console.debug('[Vue warn]: ' + msg)}

  // Register global components
  Vue.component('templated-input', require('./templated-input'));
  Vue.component('message', require('./message'));
  Vue.component('indicators', require('./indicators'));

  Vue.filter('percent', function (value, precision) {
    if (typeof precision == 'undefined') precision = 2;
    return (value * 100.0).toFixed(precision) + '%';
  });

  Vue.filter('fixed', function (value, precision) {
    if (typeof value == 'undefined') return '';
    return value.toFixed(precision)
  });

  Vue.filter('upper', function (value) {
    if (typeof value == 'undefined') return '';
    return value.toUpperCase()
  });

  // Vue app
  require('./app');
});
