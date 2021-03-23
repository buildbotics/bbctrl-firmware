/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

'use strict'


function api_cb(method, url, data, config) {
  config = $.extend({
    type: method,
    url: '/api/' + url,
    dataType: 'text',
    cache: false
  }, config);

  if (typeof data == 'object') {
    config.data = JSON.stringify(data);
    config.contentType = 'application/json; charset=utf-8';
  }

  var d = $.Deferred();

  $.ajax(config).success(function (data, status, xhr) {
    try {
      if (data) data = JSON.parse(data);

      d.resolve(data, status, xhr);

    } catch (e) {
      d.reject(data, xhr, status, 'Failed to parse JSON');
    }

  }).error(function (xhr, status, error) {
    var text = xhr.responseText;
    try {text = $.parseJSON(xhr.responseText)} catch(e) {}
    if (!text) text = error;

    d.reject(text, xhr, status, error);
    console.debug('API Error: ' + url + ': ' + text);
  });

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


  upload: function(url, data, config, progress) {
    var xhr = function () {
      var xhr = new window.XMLHttpRequest();

      if (progress) {
        xhr.upload.addEventListener('progress', function (e) {
          if (e.lengthComputable) progress(e.loaded / e.total, false, xhr);
        })
        xhr.upload.addEventListener('loadend', function (e) {
          progress(e.loaded / e.total, true, xhr);
        })
      }

      return xhr;
    }

    config = $.extend({
      processData: false,
      contentType: false,
      cache: false,
      xhr: xhr,
      data: data
    }, config);

    return api_cb('PUT', url, undefined, config);
  },


  download: function(url, type) {
    var d = $.Deferred();
    var xhr = new XMLHttpRequest();

    xhr.open('GET', '/api/' + url + '?' + Math.random(), true);
    xhr.responseType = type || 'text';
    xhr.onload = function () {
      if (200 <= xhr.status && xhr.status < 300)
        d.resolve(xhr.response, xhr.status, xhr)
      else d.reject('', xhr, xhr.status, xhr.statusText)
    }
    xhr.onerror = function () {
      d.reject('', xhr, xhr.status, xhr.statusText)
    }
    xhr.send();

    return d.promise();
  },


  'delete': function (url, config) {
    return api_cb('DELETE', url, undefined, config);
  }
}
