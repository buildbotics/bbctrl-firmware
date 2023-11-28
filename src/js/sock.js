/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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



class Sock {
  constructor(url, retry, timeout) {
    this.url       = url
    this.retry     = retry   == undefined ? 2000  : retry
    this.timeout   = timeout == undefined ? 16000 : timeout
    this.divisions = 4
    this.count     = 0

    this.connect()
  }


  onmessage() {}
  onopen()    {}
  onclose()   {}


  connect() {
    console.debug('connecting to', this.url)
    this.close()

    this._sock = new SockJS(this.url)

    this._sock.onmessage = e => {
      //console.debug('msg:', e.data)
      this.heartbeat('msg')
      this.onmessage(e)
    }


    this._sock.onopen = () => {
      console.debug('connected')
      this.heartbeat('open')
      this.onopen()
    }


    this._sock.onclose = () => {
      console.debug('disconnected')
      this._cancel_timeout()

      this.onclose()
      if (this._sock != undefined)
        setTimeout(() => this.connect(), this.retry)
    }
  }


  _timedout() {
    // Divide timeout so slow browser doesn't trigger timeouts when the
    // connection is good.
    if (this.divisions <= ++this.count) {
      console.debug('connection timedout')
      this._timeout = undefined
      this._sock.close()

    } else this._set_timeout()
  }


  _cancel_timeout() {
    clearTimeout(this._timeout)
    this._timeout = undefined
    this.count = 0
  }


  _set_timeout() {
    this._timeout = setTimeout(
      () => this._timedout(), this.timeout / this.divisions)
  }


  heartbeat(msg) {
    //console.debug('heartbeat ' + new Date().toLocaleTimeString() + ' ' + msg)
    this._cancel_timeout()
    this._set_timeout()
  }


  close() {
    if (this._sock != undefined) {
      let sock = this._sock
      this._sock = undefined
      sock.close()
    }
  }


  send(msg) {this._sock.send(msg)}
}


module.exports = Sock
