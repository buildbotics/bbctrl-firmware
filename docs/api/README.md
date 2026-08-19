# Purpose and Scope

This document describes how third-party software can connect to the Buildbotics CNC controller (Controller) over a network connection and control and monitor the controller remotely.  The document is written for software developers intending to write such third-party software.  It has been kept brief as the details are documented thoroughly in the referenced documentation.  The following sections provide an overview of the interface and its documentation.

# References

* [Buildbotics User Manual](https://buildbotics.com/manual)
* [Buildbotics Controller API Reference](https://api.buildbotics.com/)
* [Buildbotics Controller Configuration Variables](https://api.buildbotics.com/Config_Variables.md)
* [Buildbotics Controller Internal Variables](https://api.buildbotics.com/Internal_Variables.md)
* [Buildbotics Controller WebSocket API](https://api.buildbotics.com/Websocket_API.md)
* [Hypertext Transfer Protocol \-- HTTP/1.1 \- RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616)
* [The WebSocket Protocol \- RFC 6455](https://datatracker.ietf.org/doc/html/rfc6455)
* [The JavaScript Object Notation (JSON) Data Interchange Format \- RFC8259](https://datatracker.ietf.org/doc/html/rfc8259)
* [CAMotics GCode Specification](https://camotics.org/gcode.html)
* [JSON Schema](https://json-schema.org/)
* [OpenAPI Specification v3.1.0](https://spec.openapis.org/oas/v3.1.0.html)

# Glossary

* JSON  - JavaScript Object Notation
* HTTP  - HyperText Transfer Protocol
* API   - Application Programming Interface
* RFC   - Request For Comment
* CNC   - Computer Numerical Control
* GCode - The most widely used CNC programming language

# Interface Overview

Third-party clients may connect to the Buildbotics Controller either via the hardwired Ethernet port or an 802.11 Wifi connection.  Using this network connection the controller's web interface may be accessed by navigating to the controller's network address.  For example, at http://bbctrl.local/.  Making such a connection is described in the [Buildbotics User Manual](https://buildbotics.com/manual).

Over this network channel, it is also possible to access the controller programmatically using both the HTTP and WebSocket interfaces.  The API for the HTTP interface is described in the document  [Buildbotics Controller API Reference](https://api.buildbotics.com/).  The API for the WebSocket interface is described in [Buildbotics Controller WebSocket API](https://api.buildbotics.com/Websocket_API.md).  Using these APIs you can upload CNC program files (GCode), start, stop and pause programs and monitor program progress as well as the status of many other parts of the Buildbotics controller.

# Typical Usage Scenario

In the typical use case, third-party software would first establish a WebSocket connection as described in [Buildbotics Controller WebSocket API](https://api.buildbotics.com/Websocket_API.md).  This provides access to a continuously updated JSON state object from the controller.  The variables of the state object are described in [Buildbotics Controller API Reference](https://api.buildbotics.com/) with additional variables in [Buildbotics Controller Configuration Variables](https://api.buildbotics.com/Config_Variables.md) and [Buildbotics Controller Internal Variables](https://api.buildbotics.com/Internal_Variables.md).

Next a GCode file may be uploaded via the filesystem interface with an HTTP PUT request to the ``/api/fs/{path}`` endpoint where ``{path}`` is the target file name.  Next an HTTP PUT to ``/api/start/{path}`` the program.  The program can be paused vai ``/api/pause`` and stopped with ``/api/stop``.  All the while, the status can be monitored via the state object.

The API also allows for jogging the machine via ``/api/jog`` and homing the machine via ``/api/home``.

# Configuring the Controller

There are many configuration options which control the machine's performance and limits.  These may be modified programmatically by downloading a copy of the current configuration via a GET request to ``/api/load``, modifying the resulting JSON data and uploading the changes via a PUT request to ``/api/save``.  Saving the config however, requires API authorization as described below.  Configuration variables are described in [Buildbotics Controller Configuration Variables](https://api.buildbotics.com/Config_Variables.md).

# API Authorization

Log in by making a PUT request to ``/api/auth/login`` with ``user`` and ``password`` in the JSON body.  The controller replies with a session ID in an HTTP cookie named ``bbctrl-sid``.  Endpoints which require authorization may then be accessed by providing that cookie or by passing the session ID in an ``Authorization: Bearer <sid>`` header.  Sessions expire after 24 hours without use and are lost when the controller restarts.  The login status may be queried with a GET request to ``/api/auth/login``.

The built in ``admin`` user always exists, always has admin access and is authenticated with the system password of user 1000, the same password used for SSH.  It is the default when ``user`` is omitted.  Additional users are separate from the Unix system users and are created either on the Settings->Admin page or with the ``bbctrl-users`` command line tool over SSH.

When no additional users have been created, authorization is only required for a few administrative operations such as uploading a new configuration file.  Only those endpoints which specify the possibility of a 401 return code in the API documentation require authorization in this mode.

Browsers do not send the controller's cookie from another origin.  A web application served from an origin listed in the ``cors-origins`` setting may therefore log in, read the session ID from the ``sid`` field of the login response, keep it in its own storage and access the API with the ``Authorization`` header.  The session ID is only returned to allowed origins.

Once a user has been created, all endpoints, the static web files and the WebSocket require a valid session and HTTPS.  Requests made over HTTP are redirected to HTTPS.  Those endpoints which specify the possibility of a 403 return code additionally require a user with admin access.

See [https://api.buildbotics.com/\#tag/authorization/PUT/auth/login](https://api.buildbotics.com/#tag/authorization/PUT/auth/login).

# API Specification

The Buildbotics Controller API is documented in the following files:

* [Buildbotics Controller API Reference](https://api.buildbotics.com/)
* [Buildbotics Controller Configuration Variables](Config_Variables.md)
* [Buildbotics Controller Internal Variables](Internal_Variables.md)
* [Buildbotics Controller WebSocket API](Websocket_API.md)

In addition to these human friendly documents, more precise formal specifications can be found in YAML format in the following files:

* [Buildbotics Controller API Specification](https://api.buildbotics.com/bbctrl-api-1.0.yaml) OpenAPI 3.1
* [Buildbotics Controller Configuration Variable Schema](https://api.buildbotics.com/bbctrl-config-schema.yaml) JSON Schema *2020-12*
* [Buildbotics Controller Internal Variable Schema](https://api.buildbotics.com/bbctrl-vars-schema.yaml) JSON Schema *2020-12*
