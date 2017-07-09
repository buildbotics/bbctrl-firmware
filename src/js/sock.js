'use strict'


var Sock = function (url, retry, timeout) {
  if (!(this instanceof Sock)) return new Sock(url, retry);

  if (typeof retry == 'undefined') retry = 2000;
  if (typeof timeout == 'undefined') timeout = 8000;

  this.url = url;
  this.retry = retry;
  this.timeout = timeout;

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
    this._set_timeout();
    this.onmessage(e);
  }.bind(this);


  this._sock.onopen = function () {
    console.debug('connected');
    this._set_timeout();
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


Sock.prototype._set_timeout = function () {
  this._cancel_timeout();
  this._timeout = setTimeout(this._timedout.bind(this), this.timeout);
}


Sock.prototype._timedout = function () {
  console.debug('connection timedout');
  this._timeout = undefined;
  this._sock.close();
}


Sock.prototype._cancel_timeout = function () {
  clearTimeout(this._timeout);
  this._timeout = undefined;
}


Sock.prototype.close = function () {
  if (typeof this._sock != 'undefined') {
    var sock = this._sock;
    this._sock = undefined;
    sock.close();
  }
}


Sock.prototype.send = function (msg) {
  this._sock.send(msg);
}


module.exports = Sock
