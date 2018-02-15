/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
                              All rights reserved.

      This file ("the software") is free software: you can redistribute it
      and/or modify it under the terms of the GNU General Public License,
       version 2 as published by the Free Software Foundation. You should
       have received a copy of the GNU General Public License, version 2
      along with the software. If not, see <http://www.gnu.org/licenses/>.

      The software is distributed in the hope that it will be useful, but
           WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
                 License along with the software.  If not, see
                        <http://www.gnu.org/licenses/>.

                 For information regarding this software email:
                   "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

'use strict'


function api_cb(method, url, data, config) {
  config = $.extend({
    type: method,
    url: '/api/' + url,
    dataType: 'json',
    cache: false
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
    console.debug('API Error: ' + url + ': ' + xhr.responseText);
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


  upload: function(url, data, config) {
    config = $.extend({
      processData: false,
      contentType: false,
      cache: false,
      data: data
    }, config);

    return api_cb('PUT', url, undefined, config);
  },


  'delete': function (url, config) {
    return api_cb('DELETE', url, undefined, config);
  }
}
