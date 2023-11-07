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


/**
 * @author qiao        / https://github.com/qiao
 * @author mrdoob      / https://mrdoob.com
 * @author alteredq    / https://alteredqualia.com/
 * @author WestLangley / https://github.com/WestLangley
 * @author erich666    / https://erichaines.com
 * @author ScieCode    / https://github.com/sciecode
 */

// This set of controls performs orbiting, dollying (zooming), and panning.
// Unlike TrackballControls, it maintains the "up" direction object.up
// (+Y by default).
//
//    Orbit - left mouse / touch: one-finger move
//    Zoom - middle mouse, or mousewheel / touch: two-finger spread or squish
//    Pan - right mouse, or left mouse + ctrl/meta/shiftKey, or arrow keys /
//      touch: two-finger move


const STATE = {
  NONE:              -1,
  ROTATE:             0,
  DOLLY:              1,
  PAN:                2,
  TOUCH_ROTATE:       3,
  TOUCH_PAN:          4,
  TOUCH_DOLLY_PAN:    5,
  TOUCH_DOLLY_ROTATE: 6,
}

const EPS = 0.000001


module.exports = class OrbitControls extends THREE.EventDispatcher {
  constructor (obj, elem) {
    super()

    this.obj  = obj
    this.elem = elem

    // Set to false to disable this control
    this.enabled = true

    // "target" sets the location of focus, where the object orbits around
    this.target = new THREE.Vector3()

    // How far you can dolly in and out (PerspectiveCamera only)
    this.minDistance = 0
    this.maxDistance = Infinity

    // How far you can zoom in and out (OrthographicCamera only)
    this.minZoom = 0
    this.maxZoom = Infinity

    // How far you can orbit vertically, upper and lower limits.
    // Range is 0 to Math.PI radians.
    this.minPolarAngle = 0 // radians
    this.maxPolarAngle = Math.PI // radians

    // How far you can orbit horizontally, upper and lower limits.
    // If set, must be a sub-interval of the interval [ - Math.PI, Math.PI ].
    this.minAzimuthAngle = -Infinity // radians
    this.maxAzimuthAngle = Infinity  // radians

    // Set to true to enable damping (inertia)
    // If damping is enabled, you must call controls.update() in your animation
    // loop
    this.enableDamping = false
    this.dampingFactor = 0.05

    // This option actually enables dollying in and out; left as "zoom" for
    // backwards compatibility.
    // Set to false to disable zooming
    this.enableZoom = true
    this.zoomSpeed  = 1.0

    // Set to false to disable rotating
    this.enableRotate = true
    this.rotateSpeed  = 1.0

    // Set to false to disable panning
    this.enablePan          = true
    this.panSpeed           = 1.0
    this.screenSpacePanning = false // if true, pan in screen-space
    this.keyPanSpeed        = 7.0   // pixels moved per arrow key push

    // Set to true to automatically rotate around the target
    // If auto-rotate is enabled, you must call controls.update() in your
    //animation loop
    this.autoRotate      = false
    this.autoRotateSpeed = 2.0 // 30 seconds per round when fps is 60

    // Set to false to disable use of the keys
    this.enableKeys = true

    // The four arrow keys
    this.keys = {LEFT: 37, UP: 38, RIGHT: 39, BOTTOM: 40}

    // Mouse buttons
    this.mouseButtons = {
      LEFT:   THREE.MOUSE.ROTATE,
      MIDDLE: THREE.MOUSE.DOLLY,
      RIGHT:  THREE.MOUSE.PAN
    }

    // Touch fingers
    this.touches = {ONE: THREE.TOUCH.ROTATE, TWO: THREE.TOUCH.DOLLY_PAN}

    // for reset
    this.target0   = this.target.clone()
    this.position0 = this.obj.position.clone()
    this.zoom0     = this.obj.zoom

    this.offset = new THREE.Vector3()

    // so camera.up is the orbit axis
    this.quat = new THREE.Quaternion()
      .setFromUnitVectors(obj.up, new THREE.Vector3(0, 1, 0))
    this.quatInverse    = this.quat.clone().inverse()
    this.lastPosition   = new THREE.Vector3()
    this.lastQuaternion = new THREE.Quaternion()

    this.changeEvent = {type: 'change'}
    this.startEvent  = {type: 'start'}
    this.endEvent    = {type: 'end'}

    this.state = STATE.NONE

    // current position in spherical coordinates
    this.spherical      = new THREE.Spherical()
    this.sphericalDelta = new THREE.Spherical()
    this.scale          = 1
    this.panOffset      = new THREE.Vector3()
    this.zoomChanged    = false
    this.rotateStart    = new THREE.Vector2()
    this.rotateEnd      = new THREE.Vector2()
    this.rotateDelta    = new THREE.Vector2()
    this.panStart       = new THREE.Vector2()
    this.panEnd         = new THREE.Vector2()
    this.panDelta       = new THREE.Vector2()
    this.dollyStart     = new THREE.Vector2()
    this.dollyEnd       = new THREE.Vector2()
    this.dollyDelta     = new THREE.Vector2()

    elem.addEventListener('contextmenu', e => this.onContextMenu(e), false)
    elem.addEventListener('mousedown',   e => this.onMouseDown(e),   false)
    elem.addEventListener('wheel',       e => this.onMouseWheel(e),  false)
    elem.addEventListener('touchstart',  e => this.onTouchStart(e),  false)
    elem.addEventListener('touchend',    e => this.onTouchEnd(e),    false)
    elem.addEventListener('touchmove',   e => this.onTouchMove(e),   false)
    elem.addEventListener('keydown',     e => this.onKeyDown(e),     false)
    document.addEventListener('mousemove', e => this.onMouseMove(e), false)
    document.addEventListener('mouseup',   e => this.onMouseUp(e),   false)

    // make sure element can receive keys.
    if (elem.tabIndex === - 1) elem.tabIndex = 0

    // force an update at start
    this.update()
  }


  getPolarAngle() {return this.spherical.phi}
  getAzimuthalAngle() {return this.spherical.theta}


  saveState() {
    this.target0.copy(this.target)
    this.position0.copy(this.obj.position)
    this.zoom0 = this.obj.zoom
  }


  reset() {
    this.target.copy(this.target0)
    this.obj.position.copy(this.position0)
    this.obj.zoom = this.zoom0
    this.obj.updateProjectionMatrix()
    this.dispatchEvent(this.changeEvent)
    this.update()
    this.state = STATE.NONE
  }


  update() {
    let position = this.obj.position

    this.offset.copy(position).sub(this.target)

    // rotate offset to "y-axis-is-up" space
    this.offset.applyQuaternion(this.quat)

    // angle from z-axis around y-axis
    this.spherical.setFromVector3(this.offset)

    if (this.autoRotate && this.state === STATE.NONE)
      this.rotateLeft(this.getAutoRotationAngle())

    if (this.enableDamping) {
      this.spherical.theta += this.sphericalDelta.theta * this.dampingFactor
      this.spherical.phi += this.sphericalDelta.phi * this.dampingFactor

    } else {
      this.spherical.theta += this.sphericalDelta.theta
      this.spherical.phi += this.sphericalDelta.phi
    }

    // restrict theta to be between desired limits
    this.spherical.theta =
      Math.max(this.minAzimuthAngle,
               Math.min(this.maxAzimuthAngle, this.spherical.theta))

    // restrict phi to be between desired limits
    this.spherical.phi =
      Math.max(this.minPolarAngle,
               Math.min(this.maxPolarAngle, this.spherical.phi))

    this.spherical.makeSafe()

    this.spherical.radius *= this.scale

    // restrict radius to be between desired limits
    this.spherical.radius =
      Math.max(this.minDistance,
               Math.min(this.maxDistance, this.spherical.radius))

    // move target to panned location

    if (this.enableDamping === true)
      this.target.addScaledVector(this.panOffset, this.dampingFactor)
    else this.target.add(this.panOffset)

    this.offset.setFromSpherical(this.spherical)

    // rotate offset back to "camera-up-vector-is-up" space
    this.offset.applyQuaternion(this.quatInverse)

    position.copy(this.target).add(this.offset)

    this.obj.lookAt(this.target)

    if (this.enableDamping === true) {
      this.sphericalDelta.theta *= (1 - this.dampingFactor)
      this.sphericalDelta.phi *= (1 - this.dampingFactor)
      this.panOffset.multiplyScalar(1 - this.dampingFactor)

    } else {
      this.sphericalDelta.set(0, 0, 0)
      this.panOffset.set(0, 0, 0)
    }

    this.scale = 1

    // update condition is:
    // min(camera displacement, camera rotation in radians)^2 > EPS
    // using small-angle approximation cos(x/2) = 1 - x^2 / 8

    if (this.zoomChanged ||
        this.lastPosition.distanceToSquared(this.obj.position) > EPS ||
        8 * (1 - this.lastQuaternion.dot(this.obj.quaternion)) > EPS) {

      this.dispatchEvent(this.changeEvent)

      this.lastPosition.copy(this.obj.position)
      this.lastQuaternion.copy(this.obj.quaternion)
      this.zoomChanged = false

      return true
    }

    return false
  }


  getAutoRotationAngle() {
    return 2 * Math.PI / 60 / 60 * this.autoRotateSpeed
  }


  getZoomScale() {return Math.pow(0.95, this.zoomSpeed)}
  rotateLeft(angle) {this.sphericalDelta.theta -= angle}
  rotateUp(angle) {this.sphericalDelta.phi -= angle}


  panLeft(distance, objectMatrix) {
    let v = new THREE.Vector3()
    v.setFromMatrixColumn(objectMatrix, 0) // get X column of objectMatrix
    v.multiplyScalar(- distance)
    this.panOffset.add(v)
  }


  panUp(distance, objectMatrix) {
    let v = new THREE.Vector3()

    if (this.screenSpacePanning === true)
      v.setFromMatrixColumn(objectMatrix, 1)

    else {
      v.setFromMatrixColumn(objectMatrix, 0)
      v.crossVectors(this.obj.up, v)
    }

    v.multiplyScalar(distance)
    this.panOffset.add(v)
  }


  // deltaX and deltaY are in pixels; right and down are positive
  pan(deltaX, deltaY) {
    let offset = new THREE.Vector3()
    let element = this.elem

    if (this.obj.isPerspectiveCamera) {
      // perspective
      let position = this.obj.position
      offset.copy(position).sub(this.target)
      let targetDistance = offset.length()

      // half of the fov is center to top of screen
      targetDistance *= Math.tan((this.obj.fov / 2) * Math.PI / 180.0)

      // we use only clientHeight here so aspect ratio does not distort speed
      this.panLeft(2 * deltaX * targetDistance / element.clientHeight,
                   this.obj.matrix)
      this.panUp(2 * deltaY * targetDistance / element.clientHeight,
                 this.obj.matrix)

    } else if (this.obj.isOrthographicCamera) {
      // orthographic
      this.panLeft(deltaX * (this.obj.right - this.obj.left) /
                   this.obj.zoom / element.clientWidth, this.obj.matrix)
      this.panUp(deltaY * (this.obj.top - this.obj.bottom) /
                 this.obj.zoom / element.clientHeight, this.obj.matrix)

    } else {
      // camera neither orthographic nor perspective
      console.warn('WARNING: OrbitControls encountered an unknown ' +
                   'camera type - pan disabled.')
      this.enablePan = false
    }
  }


  dollyIn(dollyScale) {
    if (this.obj.isPerspectiveCamera) this.scale /= dollyScale
    else if (this.obj.isOrthographicCamera) {
      this.obj.zoom =
        Math.max(this.minZoom,
                 Math.min(this.maxZoom, this.obj.zoom * dollyScale))
      this.obj.updateProjectionMatrix()
      this.zoomChanged = true

    } else {
      console.warn('WARNING: OrbitControls encountered an unknown camera ' +
                   'type - dolly/zoom disabled.')
      this.enableZoom = false
    }
  }


  dollyOut(dollyScale) {
    if (this.obj.isPerspectiveCamera) this.scale *= dollyScale
    else if (this.obj.isOrthographicCamera) {
      this.obj.zoom =
        Math.max(this.minZoom,
                 Math.min(this.maxZoom, this.obj.zoom / dollyScale))
      this.obj.updateProjectionMatrix()
      this.zoomChanged = true

    } else {
      console.warn('WARNING: OrbitControls encountered an unknown camera ' +
                   'type - dolly/zoom disabled.')
      this.enableZoom = false
    }
  }


  // event callbacks - update the object state
  handleMouseDownRotate(event) {
    this.rotateStart.set(event.clientX, event.clientY)
  }


  handleMouseDownDolly(event) {
    this.dollyStart.set(event.clientX, event.clientY)
  }


  handleMouseDownPan(event) {
    this.panStart.set(event.clientX, event.clientY)
  }


  handleMouseMoveRotate(event) {
    this.rotateEnd.set(event.clientX, event.clientY)
    this.rotateDelta.subVectors(this.rotateEnd, this.rotateStart)
      .multiplyScalar(this.rotateSpeed)

    let element = this.elem
    this.rotateLeft(2 * Math.PI * this.rotateDelta.x / element.clientHeight)
    this.rotateUp(2 * Math.PI * this.rotateDelta.y / element.clientHeight)
    this.rotateStart.copy(this.rotateEnd)
    this.update()
  }


  handleMouseMoveDolly(event) {
    this.dollyEnd.set(event.clientX, event.clientY)
    this.dollyDelta.subVectors(this.dollyEnd, this.dollyStart)

    if (this.dollyDelta.y > 0) this.dollyIn(this.getZoomScale())
    else if (this.dollyDelta.y < 0) this.dollyOut(this.getZoomScale())

    this.dollyStart.copy(this.dollyEnd)
    this.update()
  }


  handleMouseMovePan(event) {
    this.panEnd.set(event.clientX, event.clientY)
    this.panDelta.subVectors(this.panEnd, this.panStart)
      .multiplyScalar(this.panSpeed)
    this.pan(this.panDelta.x, this.panDelta.y)
    this.panStart.copy(this.panEnd)
    this.update()
  }


  handleMouseUp() {} // no-op


  handleMouseWheel(event) {
    if (event.deltaY < 0) this.dollyOut(this.getZoomScale())
    else if (event.deltaY > 0) this.dollyIn(this.getZoomScale())
    this.update()
  }


  handleKeyDown(event) {
    let needsUpdate = false

    switch (event.keyCode) {
    case this.keys.UP:
      this.pan(0, this.keyPanSpeed)
      needsUpdate = true
      break

    case this.keys.BOTTOM:
      this.pan(0, - this.keyPanSpeed)
      needsUpdate = true
      break

    case this.keys.LEFT:
      this.pan(this.keyPanSpeed, 0)
      needsUpdate = true
      break

    case this.keys.RIGHT:
      this.pan(-this.keyPanSpeed, 0)
      needsUpdate = true
      break
    }

    if (needsUpdate) {
      // prevent the browser from scrolling on cursor keys
      event.preventDefault()
      this.update()
    }
  }


  handleTouchStartRotate(event) {
    if (event.touches.length == 1)
      this.rotateStart.set(event.touches[ 0 ].pageX, event.touches[ 0 ].pageY)

    else {
      let x = 0.5 * (event.touches[ 0 ].pageX + event.touches[ 1 ].pageX)
      let y = 0.5 * (event.touches[ 0 ].pageY + event.touches[ 1 ].pageY)
      this.rotateStart.set(x, y)
    }
  }


  handleTouchStartPan(event) {
    if (event.touches.length == 1)
      this.panStart.set(event.touches[ 0 ].pageX, event.touches[ 0 ].pageY)

    else {
      let x = 0.5 * (event.touches[ 0 ].pageX + event.touches[ 1 ].pageX)
      let y = 0.5 * (event.touches[ 0 ].pageY + event.touches[ 1 ].pageY)
      this.panStart.set(x, y)
    }
  }


  handleTouchStartDolly(event) {
    let dx = event.touches[ 0 ].pageX - event.touches[ 1 ].pageX
    let dy = event.touches[ 0 ].pageY - event.touches[ 1 ].pageY
    let distance = Math.sqrt(dx * dx + dy * dy)
    this.dollyStart.set(0, distance)
  }


  handleTouchStartDollyPan(event) {
    if (this.enableZoom) this.handleTouchStartDolly(event)
    if (this.enablePan) this.handleTouchStartPan(event)
  }


  handleTouchStartDollyRotate(event) {
    if (this.enableZoom) this.handleTouchStartDolly(event)
    if (this.enableRotate) this.handleTouchStartRotate(event)
  }


  handleTouchMoveRotate(event) {
    if (event.touches.length == 1)
      this.rotateEnd.set(event.touches[ 0 ].pageX, event.touches[ 0 ].pageY)

    else {
      let x = 0.5 * (event.touches[ 0 ].pageX + event.touches[ 1 ].pageX)
      let y = 0.5 * (event.touches[ 0 ].pageY + event.touches[ 1 ].pageY)
      this.rotateEnd.set(x, y)
    }

    this.rotateDelta.subVectors(this.rotateEnd, this.rotateStart)
      .multiplyScalar(this.rotateSpeed)

    let element = this.elem
    this.rotateLeft(2 * Math.PI * this.rotateDelta.x / element.clientHeight)
    this.rotateUp(2 * Math.PI * this.rotateDelta.y / element.clientHeight)
    this.rotateStart.copy(this.rotateEnd)
  }


  handleTouchMovePan(event) {
    if (event.touches.length == 1)
      this.panEnd.set(event.touches[ 0 ].pageX, event.touches[ 0 ].pageY)

    else {
      let x = 0.5 * (event.touches[ 0 ].pageX + event.touches[ 1 ].pageX)
      let y = 0.5 * (event.touches[ 0 ].pageY + event.touches[ 1 ].pageY)
      this.panEnd.set(x, y)
    }

    this.panDelta.subVectors(this.panEnd, this.panStart)
      .multiplyScalar(this.panSpeed)
    this.pan(this.panDelta.x, this.panDelta.y)
    this.panStart.copy(this.panEnd)
  }


  handleTouchMoveDolly(event) {
    let dx = event.touches[ 0 ].pageX - event.touches[ 1 ].pageX
    let dy = event.touches[ 0 ].pageY - event.touches[ 1 ].pageY
    let distance = Math.sqrt(dx * dx + dy * dy)

    this.dollyEnd.set(0, distance)
    this.dollyDelta.set(
      0, Math.pow(this.dollyEnd.y / this.dollyStart.y, this.zoomSpeed))
    this.dollyIn(this.dollyDelta.y)
    this.dollyStart.copy(this.dollyEnd)
  }


  handleTouchMoveDollyPan(event) {
    if (this.enableZoom) this.handleTouchMoveDolly(event)
    if (this.enablePan) this.handleTouchMovePan(event)
  }


  handleTouchMoveDollyRotate(event) {
    if (this.enableZoom) this.handleTouchMoveDolly(event)
    if (this.enableRotate) this.handleTouchMoveRotate(event)
  }


  handleTouchEnd() {} // no-op


  // event handlers - FSM: listen for events and reset state
  onMouseDown(event) {
    if (this.enabled === false) return

    // Prevent the browser from scrolling.
    event.preventDefault()

    // Manually set the focus since calling preventDefault above
    // prevents the browser from setting it automatically.

    if (this.elem.focus) this.elem.focus()
    else window.focus()

    switch (event.button) {
    case 0:
      switch (this.mouseButtons.LEFT) {
      case THREE.MOUSE.ROTATE:
        if (event.ctrlKey || event.metaKey || event.shiftKey) {
          if (this.enablePan === false) return
          this.handleMouseDownPan(event)
          this.state = STATE.PAN

        } else {
          if (this.enableRotate === false) return
          this.handleMouseDownRotate(event)
          this.state = STATE.ROTATE
        }
        break

      case THREE.MOUSE.PAN:
        if (event.ctrlKey || event.metaKey || event.shiftKey) {
          if (this.enableRotate === false) return
          this.handleMouseDownRotate(event)
          this.state = STATE.ROTATE

        } else {
          if (this.enablePan === false) return
          this.handleMouseDownPan(event)
          this.state = STATE.PAN
        }
        break

      default: this.state = STATE.NONE
      }
      break

    case 1:
      switch (this.mouseButtons.MIDDLE) {
      case THREE.MOUSE.DOLLY:
        if (this.enableZoom === false) return
        this.handleMouseDownDolly(event)
        this.state = STATE.DOLLY
        break

      default: this.state = STATE.NONE
      }
      break

    case 2:
      switch (this.mouseButtons.RIGHT) {
      case THREE.MOUSE.ROTATE:
        if (this.enableRotate === false) return
        this.handleMouseDownRotate(event)
        this.state = STATE.ROTATE
        break

      case THREE.MOUSE.PAN:
        if (this.enablePan === false) return
        this.handleMouseDownPan(event)
        this.state = STATE.PAN
        break

      default: this.state = STATE.NONE
      }
      break
    }

    if (this.state !== STATE.NONE)
      this.dispatchEvent(this.startEvent)
  }


  onMouseMove(event) {
    if (this.enabled === false || this.state == STATE.NONE) return
    event.preventDefault()

    switch (this.state) {
    case STATE.ROTATE:
      if (this.enableRotate === false) return
      this.handleMouseMoveRotate(event)
      break

    case STATE.DOLLY:
      if (this.enableZoom === false) return
      this.handleMouseMoveDolly(event)
      break

    case STATE.PAN:
      if (this.enablePan === false) return
      this.handleMouseMovePan(event)
      break
    }
  }


  onMouseUp(event) {
    if (this.enabled === false || this.state == STATE.NONE) return

    this.handleMouseUp(event)
    this.dispatchEvent(this.endEvent)
    this.state = STATE.NONE
  }


  onMouseWheel(event) {
    if (this.enabled === false || this.enableZoom === false ||
        (this.state !== STATE.NONE && this.state !== STATE.ROTATE)) return

    event.preventDefault()
    event.stopPropagation()
    this.dispatchEvent(this.startEvent)
    this.handleMouseWheel(event)
    this.dispatchEvent(this.endEvent)
  }


  onKeyDown(event) {
    if (this.enabled === false || this.enableKeys === false ||
        this.enablePan === false) return

    this.handleKeyDown(event)
  }


  onTouchStart(event) {
    if (this.enabled === false) return
    event.preventDefault()

    switch (event.touches.length) {
    case 1:
      switch (this.touches.ONE) {
      case THREE.TOUCH.ROTATE:
        if (this.enableRotate === false) return
        this.handleTouchStartRotate(event)
        this.state = STATE.TOUCH_ROTATE
        break

      case THREE.TOUCH.PAN:
        if (this.enablePan === false) return
        this.handleTouchStartPan(event)
        this.state = STATE.TOUCH_PAN
        break

      default: this.state = STATE.NONE
      }
      break

    case 2:
      switch (this.touches.TWO) {
      case THREE.TOUCH.DOLLY_PAN:
        if (this.enableZoom === false && this.enablePan === false) return
        this.handleTouchStartDollyPan(event)
        this.state = STATE.TOUCH_DOLLY_PAN
        break

      case THREE.TOUCH.DOLLY_ROTATE:
        if (this.enableZoom === false && this.enableRotate === false) return
        this.handleTouchStartDollyRotate(event)
        this.state = STATE.TOUCH_DOLLY_ROTATE
        break

      default: this.state = STATE.NONE
      }
      break

    default: this.state = STATE.NONE
    }

    if (this.state !== STATE.NONE) this.dispatchEvent(this.startEvent)
  }


  onTouchMove(event) {
    if (this.enabled === false) return

    event.preventDefault()
    event.stopPropagation()

    switch (this.state) {
    case STATE.TOUCH_ROTATE:
      if (this.enableRotate === false) return
      this.handleTouchMoveRotate(event)
      this.update()
      break

    case STATE.TOUCH_PAN:
      if (this.enablePan === false) return
      this.handleTouchMovePan(event)
      this.update()
      break

    case STATE.TOUCH_DOLLY_PAN:
      if (this.enableZoom === false && this.enablePan === false) return
      this.handleTouchMoveDollyPan(event)
      this.update()
      break

    case STATE.TOUCH_DOLLY_ROTATE:
      if (this.enableZoom === false && this.enableRotate === false) return
      this.handleTouchMoveDollyRotate(event)
      this.update()
      break

    default: this.state = STATE.NONE
    }
  }


  onTouchEnd(event) {
    if (this.enabled === false) return

    this.handleTouchEnd(event)
    this.dispatchEvent(this.endEvent)
    this.state = STATE.NONE
  }


  onContextMenu(event) {
    if (this.enabled === false) return
    event.preventDefault()
  }
}
