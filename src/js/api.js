'use strict'


function api_cb(method, url, data, config) {
  config = $.extend({
    type: method,
    url: '/api/' + url,
    dataType: 'json'
  }, config);

  if (typeof data == 'object') {
    config.data = JSON.stringify(data);
    config.contentType = 'application/json; charset=utf-8';
  }

  var d = $.Deferred();

  $.ajax(config).success(function (data, status, xhr) {
    d.resolve(data, status, xhr);

  }).error(function (xhr, status, error) {
    var text = xhr.responseText;
    try {text = $.parseJSON(xhr.responseText)} catch(e) {}
    d.reject(text, xhr, status, error);
    console.debug('API Error: ' + url + ': ' + text);
  })

  return d.promise();
}


module.exports = {
  get: function (url, config) {
    return api_cb('GET', url, undefined, config);
  },


  put: function(url, data, config) {
    return api_cb('PUT', url, data, config);
  },


  post: function(url, data, config) {
    return api_cb('POST', url, data, config);
  },


  'delete': function (url, config) {
    return api_cb('DELETE', url, undefined, config);
  }
}
