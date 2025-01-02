# Buildbotics Controller Websocket API

The Buildbotics Controller API is described in detail at
https://api.buildbotics.com/  This document details the messages exchanged
by over the controller's Websocket endpoint at ``/api/websocket``.

## Overview

After connecting to the controller's Websocket endpoint at
ws://bbctrl.local/api/websocket according to
[The WebSocket Protocol](https://datatracker.ietf.org/doc/html/rfc6455),
the entire current state of the Buildbotics controller will be transmitted
to the connected client as a JSON object.  The format of the state object is
defined in https://api.buildbotics.com/#model/state.  Additional state
variables are defined in
https://api.buildbotics.com/bbctrl-config-schema.yaml and
https://api.buildbotics.com/bbctrl-vars-schema.yaml.

After the initial state message, the controller will send updates to the
state object as described in the next section.

Messages sent from the client to the controller via the Websocket are
treated as GCode MDI commands and executed immediately on the controller if
possible.

## Update Messages

After the initial complete controller state is sent, the client will receive
update messages.  These messages are JSON objects which should be merged with
the state object to get the new state.  Any key in the update object is
inserted into the state object replacing any previous value in the state
object unless both the child values are themselves objects.  In such case,
the child objects are merged in the same manner.

The following Javascript code demonstrates the update process:

```javascript
is_object(o) {return o !== null && typeof o == 'object'},

update_state(state, update) {
  for (const [key, value] of Object.entries(update))
    if (is_object(value) && is_object(state[key]))
      update_state(state[key], value)
    else state[key] = value
}
```

## Example Websocket handler
The following code uses the web browser WebSocket API to connect
to a Buildbotics Controller at http://bbctrl.local.  Also see
https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API.

```javascript
let state
ws = new WebSocket('ws://bbctrl.local/websocket')

ws.onmessage(event) {
  let msg = JSON.parse(event.data)
  if (state == undefined) state = msg  // First message
  else update_state(state, msg)        // Update messages
}
```
