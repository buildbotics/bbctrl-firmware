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


var Sock = function (url, retry, timeout) {
  if (!(this instanceof Sock)) return new Sock(url, retry);

  if (typeof retry == 'undefined') retry = 2000;
  if (typeof timeout == 'undefined') timeout = 16000;

  this.url = url;
  this.retry = retry;
  this.timeout = timeout;
  this.divisions = 4;
  this.count = 0;

  this.connect();
}


Sock.prototype.onmessage = function () {}
Sock.prototype.onopen = function () {}
Sock.prototype.onclose = function () {}


Sock.prototype.connect = function () {
  console.debug('connecting to', this.url);
  this.close();

  this._sock = new SockJS(this.url);

  this._sock.onmessage = function (e) {
    console.debug('msg:', e.data);
    this.heartbeat('msg');
    this.onmessage(e);
  }.bind(this);


  this._sock.onopen = function () {
    console.debug('connected');
    this.heartbeat('open');
    this.onopen();
  }.bind(this);


  this._sock.onclose = function () {
    console.debug('disconnected');
    this._cancel_timeout();

    this.onclose();
    if (typeof this._sock != 'undefined')
      setTimeout(this.connect.bind(this), this.retry);
  }.bind(this);
}


Sock.prototype._timedout = function () {
  // Divide timeout so slow browser doesn't trigger timeouts when the
  // connection is good.
  if (this.divisions <= ++this.count) {
    console.debug('connection timedout');
    this._timeout = undefined;
    this._sock.close();

  } else this._set_timeout();
}


Sock.prototype._cancel_timeout = function () {
  clearTimeout(this._timeout);
  this._timeout = undefined;
  this.count = 0;
}


Sock.prototype._set_timeout = function () {
  this._timeout = setTimeout(this._timedout.bind(this),
                             this.timeout / this.divisions);
}


Sock.prototype.heartbeat = function (msg) {
  //console.debug('heartbeat ' + new Date().toLocaleTimeString() + ' ' + msg);
  this._cancel_timeout();
  this._set_timeout();
}


Sock.prototype.close = function () {
  if (typeof this._sock != 'undefined') {
    var sock = this._sock;
    this._sock = undefined;
    sock.close();
  }
}


Sock.prototype.send = function (msg) {this._sock.send(msg)}


module.exports = Sock
